#include "dv/server/server.h"
#include "dv/server/request_context.h"
#include "dv/collection.h"
#include "dv/filter.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <deque>
#include <thread>
#include <mutex>
#include <unordered_map>

using json = nlohmann::json;

namespace dv {
namespace server {

static void setNonBlocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static std::string statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

static std::string buildResponse(int status, const std::string& content_type, const std::string& body) {
    std::ostringstream os;
    os << "HTTP/1.1 " << status << " " << statusText(status) << "\r\n";
    os << "Content-Type: " << content_type << "\r\n";
    os << "Content-Length: " << body.size() << "\r\n";
    os << "Connection: keep-alive\r\n";
    os << "Server: DeepVector/0.1.0\r\n";
    os << "\r\n";
    os << body;
    return os.str();
}

static bool checkAuth(const std::string& request, const std::string& api_key) {
    if (api_key.empty()) return true;
    auto pos = request.find("Authorization: Bearer ");
    if (pos == std::string::npos) return false;
    pos += 22;
    auto end = request.find("\r\n", pos);
    return request.substr(pos, end - pos) == api_key;
}

static std::string methodFromRequest(const std::string& raw) {
    auto sp = raw.find(' ');
    if (sp == std::string::npos) return "";
    return raw.substr(0, sp);
}

static std::string pathFromRequest(const std::string& raw) {
    auto sp1 = raw.find(' ');
    if (sp1 == std::string::npos) return "";
    auto sp2 = raw.find(' ', sp1 + 1);
    return raw.substr(sp1 + 1, sp2 - sp1 - 1);
}

static std::string pathOnly(const std::string& path) {
    auto q = path.find('?');
    return q == std::string::npos ? path : path.substr(0, q);
}

static std::string queryParam(const std::string& path, const std::string& key) {
    auto q = path.find('?');
    if (q == std::string::npos) return "";
    std::string qs = path.substr(q + 1);
    std::string prefix = key + "=";
    auto pos = qs.find(prefix);
    if (pos == std::string::npos) return "";
    auto start = pos + prefix.size();
    auto end = qs.find('&', start);
    return qs.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

static std::string bodyFromRequest(const std::string& raw) {
    auto pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return raw.substr(pos + 4);
}

static storage::DocumentMeta metaFromJson(const json& meta_j) {
    storage::DocumentMeta meta;
    meta.text = meta_j.value("text", "");
    meta.tags = meta_j.value("tags", "");
    meta.timestamp = meta_j.value("timestamp", static_cast<int64_t>(0));
    return meta;
}

static void appendResultItem(json& arr, const SearchResult& r, Collection* coll) {
    json item;
    item["id"] = r.id;
    item["distance"] = r.distance;
    if (auto meta = coll->getMeta(r.id)) {
        item["text"] = meta->text;
        item["tags"] = meta->tags;
        item["timestamp"] = meta->timestamp;
    }
    arr.push_back(item);
}

static std::string collectionNameFrom(const json& req, const std::string& path_full,
                                      const std::string& fallback) {
    if (req.is_object() && req.contains("collection") && req["collection"].is_string()) {
        return req["collection"].get<std::string>();
    }
    auto q = queryParam(path_full, "collection");
    return q.empty() ? fallback : q;
}

class DeepVectorServer::Impl {
public:
    Impl(DeepVectorServer* owner, const ServerConfig& cfg, ServerStats& stats)
        : owner_(owner), config_(cfg), stats_(stats),
          listen_fd_(-1), running_(false) {
        getOrCreate(config_.default_collection);
    }

    ~Impl() { stop(); }

    Collection* getOrCreate(const std::string& name) {
        return owner_->registry_->getOrCreate(name);
    }

    Collection* getExisting(const std::string& name) {
        return owner_->registry_->get(name);
    }

    std::vector<std::string> listNames() {
        return owner_->registry_->list();
    }

    void start() {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ < 0) {
            std::cerr << "DeepVector: failed to create socket" << std::endl;
            return;
        }
        int opt = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setNonBlocking(listen_fd_);

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(config_.port);
        ::inet_pton(AF_INET, config_.host.c_str(), &addr.sin_addr);

        if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "DeepVector: failed to bind to " << config_.host << ":"
                      << config_.port << std::endl;
            ::close(listen_fd_);
            listen_fd_ = -1;
            return;
        }
        ::listen(listen_fd_, 128);

        running_ = true;
        std::cout << "DeepVector server listening on " << config_.host << ":"
                  << config_.port << std::endl;

        serverThread_ = std::thread([this] { eventLoop(); });
    }

    void stop() {
        running_ = false;
        if (serverThread_.joinable()) serverThread_.join();
        for (auto fd : client_fds_) ::close(fd);
        client_fds_.clear();
        if (listen_fd_ >= 0) {
            ::close(listen_fd_);
            listen_fd_ = -1;
        }
    }

private:
    void eventLoop() {
        std::deque<int> clients;
        char buf[65536];

        while (running_) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(listen_fd_, &readfds);
            int maxfd = listen_fd_;
            for (int fd : clients) {
                FD_SET(fd, &readfds);
                if (fd > maxfd) maxfd = fd;
            }

            struct timeval tv{1, 0};
            int n = ::select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }

            if (FD_ISSET(listen_fd_, &readfds)) {
                struct sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                int cfd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
                if (cfd >= 0) {
                    setNonBlocking(cfd);
                    clients.push_back(cfd);
                    stats_.active_connections++;
                }
            }

            for (auto it = clients.begin(); it != clients.end();) {
                int fd = *it;
                if (!FD_ISSET(fd, &readfds)) {
                    ++it;
                    continue;
                }
                ssize_t nread = ::read(fd, buf, sizeof(buf) - 1);
                if (nread <= 0) {
                    ::close(fd);
                    stats_.active_connections--;
                    it = clients.erase(it);
                    continue;
                }
                buf[nread] = '\0';
                std::string response = handleRequest(std::string(buf, nread));
                ::write(fd, response.data(), response.size());
                ++it;
            }
        }

        for (int fd : clients) {
            ::close(fd);
            stats_.active_connections--;
        }
    }

    std::string handleRequest(const std::string& raw) {
        stats_.total_requests++;

        std::string method = methodFromRequest(raw);
        std::string path_full = pathFromRequest(raw);
        std::string path = pathOnly(path_full);
        std::string body = bodyFromRequest(raw);

        if (method.empty() || path.empty()) {
            stats_.error_requests++;
            json err;
            err["error"] = "bad request";
            return buildResponse(400, "application/json", err.dump());
        }

        if (path != "/health" && !checkAuth(raw, config_.api_key)) {
            stats_.error_requests++;
            json err;
            err["error"] = "unauthorized";
            return buildResponse(401, "application/json", err.dump());
        }

        try {
            json req = json::object();
            if (!body.empty()) {
                try { req = json::parse(body); } catch (...) { req = json::object(); }
            }

            // POST /collections  {"name":"foo","dim"?:384}
            if (path == "/collections" && method == "POST") {
                std::string name = req.value("name", "");
                if (name.empty()) {
                    json err;
                    err["error"] = "name is required";
                    return buildResponse(400, "application/json", err.dump());
                }
                Collection* coll = getOrCreate(name);
                json resp;
                resp["name"] = name;
                resp["vectors"] = coll->size();
                resp["dim"] = coll->dim();
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/collections" && method == "GET") {
                json resp;
                resp["collections"] = json::array();
                for (const auto& name : listNames()) {
                    Collection* coll = getExisting(name);
                    if (!coll) continue;
                    json c;
                    c["name"] = name;
                    c["vectors"] = coll->size();
                    c["dim"] = coll->dim();
                    resp["collections"].push_back(c);
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            // DELETE /collections/:name
            if (method == "DELETE" && path.rfind("/collections/", 0) == 0) {
                std::string name = path.substr(13);
                if (name == config_.default_collection) {
                    json err;
                    err["error"] = "cannot delete default collection";
                    return buildResponse(400, "application/json", err.dump());
                }
                owner_->registry_->erase(name);
                json resp;
                resp["status"] = "ok";
                resp["name"] = name;
                return buildResponse(200, "application/json", resp.dump());
            }

            auto resolveColl = [&](const json& r) -> Collection* {
                std::string name = collectionNameFrom(r, path_full, config_.default_collection);
                return getOrCreate(name);
            };

            if (path == "/batch/search" && method == "POST") {
                stats_.search_requests++;
                Collection* coll = resolveColl(req);
                json resp;
                resp["results"] = json::array();

                for (auto& q : req["queries"]) {
                    Collection* c = q.contains("collection")
                        ? getOrCreate(q["collection"].get<std::string>())
                        : coll;
                    std::vector<float> vec = q["vector"].get<std::vector<float>>();
                    size_t k = q.value("k", 10);
                    std::vector<SearchResult> results;
                    if (q.contains("filter")) {
                        results = c->searchWithFilter(vec.data(), k, buildFilter(q["filter"]));
                    } else {
                        results = c->search(vec.data(), k);
                    }
                    json batch_results;
                    batch_results["results"] = json::array();
                    for (auto& r : results) {
                        appendResultItem(batch_results["results"], r, c);
                    }
                    resp["results"].push_back(batch_results);
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            if (method == "GET" && (path.rfind("/vectors/", 0) == 0 || path == "/vector")) {
                Collection* coll = getOrCreate(
                    queryParam(path_full, "collection").empty()
                        ? config_.default_collection
                        : queryParam(path_full, "collection"));

                uint64_t id = 0;
                bool want_meta = false;

                if (path == "/vector") {
                    auto id_str = queryParam(path_full, "id");
                    if (id_str.empty()) {
                        json err;
                        err["error"] = "missing id";
                        return buildResponse(400, "application/json", err.dump());
                    }
                    id = std::stoull(id_str);
                } else {
                    auto rest = path.substr(9);
                    auto slash = rest.find('/');
                    id = std::stoull(slash == std::string::npos ? rest : rest.substr(0, slash));
                    if (slash != std::string::npos && rest.substr(slash) == "/meta") {
                        want_meta = true;
                    }
                }

                if (want_meta) {
                    auto meta = coll->getMeta(id);
                    if (!meta) {
                        json err;
                        err["error"] = "not found";
                        return buildResponse(404, "application/json", err.dump());
                    }
                    json resp;
                    resp["id"] = id;
                    resp["text"] = meta->text;
                    resp["tags"] = meta->tags;
                    resp["timestamp"] = meta->timestamp;
                    return buildResponse(200, "application/json", resp.dump());
                }

                const float* vec = coll->getVector(id);
                if (!vec) {
                    json err;
                    err["error"] = "vector not found";
                    return buildResponse(404, "application/json", err.dump());
                }
                json resp;
                resp["id"] = id;
                resp["vector"] = json::array();
                for (Dimension d = 0; d < coll->dim(); d++) {
                    resp["vector"].push_back(vec[d]);
                }
                if (auto meta = coll->getMeta(id)) {
                    resp["text"] = meta->text;
                    resp["tags"] = meta->tags;
                    resp["timestamp"] = meta->timestamp;
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/health" && method == "GET") {
                Collection* coll = getOrCreate(config_.default_collection);
                json resp;
                resp["status"] = "ok";
                resp["vectors"] = coll->size();
                resp["dim"] = coll->dim();
                resp["collections"] = listNames().size();
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/stats" && method == "GET") {
                json resp;
                resp["requests"] = stats_.total_requests.load();
                resp["searches"] = stats_.search_requests.load();
                resp["inserts"] = stats_.insert_requests.load();
                resp["errors"] = stats_.error_requests.load();
                resp["connections"] = stats_.active_connections.load();
                return buildResponse(200, "application/json", resp.dump());
            }

            // Prometheus-compatible text metrics (enterprise observability baseline)
            if (path == "/metrics" && method == "GET") {
                auto searches = stats_.search_requests.load();
                auto inserts = stats_.insert_requests.load();
                double avg_search = searches
                    ? (stats_.search_latency_sum_us.load() / 1000.0) / searches
                    : 0.0;
                double avg_insert = inserts
                    ? (stats_.insert_latency_sum_us.load() / 1000.0) / inserts
                    : 0.0;
                std::ostringstream m;
                m << "# HELP deepvector_requests_total Total HTTP requests\n"
                  << "# TYPE deepvector_requests_total counter\n"
                  << "deepvector_requests_total " << stats_.total_requests.load() << "\n"
                  << "# HELP deepvector_searches_total Total search requests\n"
                  << "# TYPE deepvector_searches_total counter\n"
                  << "deepvector_searches_total " << searches << "\n"
                  << "# HELP deepvector_inserts_total Total insert requests\n"
                  << "# TYPE deepvector_inserts_total counter\n"
                  << "deepvector_inserts_total " << inserts << "\n"
                  << "# HELP deepvector_errors_total Total error responses\n"
                  << "# TYPE deepvector_errors_total counter\n"
                  << "deepvector_errors_total " << stats_.error_requests.load() << "\n"
                  << "# HELP deepvector_active_connections Current connections\n"
                  << "# TYPE deepvector_active_connections gauge\n"
                  << "deepvector_active_connections " << stats_.active_connections.load() << "\n"
                  << "# HELP deepvector_collections Current collections\n"
                  << "# TYPE deepvector_collections gauge\n"
                  << "deepvector_collections " << owner_->registry_->size() << "\n"
                  << "# HELP deepvector_search_latency_avg_ms Average search latency\n"
                  << "# TYPE deepvector_search_latency_avg_ms gauge\n"
                  << "deepvector_search_latency_avg_ms " << avg_search << "\n"
                  << "# HELP deepvector_insert_latency_avg_ms Average insert latency\n"
                  << "# TYPE deepvector_insert_latency_avg_ms gauge\n"
                  << "deepvector_insert_latency_avg_ms " << avg_insert << "\n";
                return buildResponse(200, "text/plain; version=0.0.4", m.str());
            }

            if (path == "/search" && method == "POST") {
                stats_.search_requests++;
                auto t0 = RequestContext::nowMs();
                Collection* coll = resolveColl(req);
                std::vector<float> vec = req["vector"].get<std::vector<float>>();
                size_t k = req.value("k", 10);

                std::vector<SearchResult> results;
                if (req.contains("filter")) {
                    results = coll->searchWithFilter(vec.data(), k, buildFilter(req["filter"]));
                } else {
                    results = coll->search(vec.data(), k);
                }

                stats_.search_latency_sum_us +=
                    static_cast<uint64_t>((RequestContext::nowMs() - t0) * 1000);

                json resp;
                resp["collection"] = collectionNameFrom(req, path_full, config_.default_collection);
                resp["results"] = json::array();
                for (auto& r : results) {
                    appendResultItem(resp["results"], r, coll);
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/insert" && method == "POST") {
                stats_.insert_requests++;
                auto t0 = RequestContext::nowMs();
                Collection* coll = resolveColl(req);
                json resp;
                resp["collection"] = collectionNameFrom(req, path_full, config_.default_collection);
                resp["ids"] = json::array();

                auto insert_one = [&](const std::vector<float>& vec, const json* meta_j) {
                    if (meta_j && !meta_j->is_null()) {
                        return coll->add(vec.data(), metaFromJson(*meta_j));
                    }
                    return coll->add(vec.data());
                };

                if (req.contains("vectors")) {
                    const json* metas = req.contains("metadatas") ? &req["metadatas"] : nullptr;
                    size_t i = 0;
                    for (auto& v : req["vectors"]) {
                        std::vector<float> vec = v.get<std::vector<float>>();
                        const json* meta_j = nullptr;
                        if (metas && metas->is_array() && i < metas->size()) {
                            meta_j = &(*metas)[i];
                        }
                        resp["ids"].push_back(insert_one(vec, meta_j));
                        ++i;
                    }
                } else if (req.contains("vector")) {
                    std::vector<float> vec = req["vector"].get<std::vector<float>>();
                    const json* meta_j = req.contains("meta") ? &req["meta"] : nullptr;
                    resp["ids"].push_back(insert_one(vec, meta_j));
                }
                stats_.insert_latency_sum_us +=
                    static_cast<uint64_t>((RequestContext::nowMs() - t0) * 1000);
                return buildResponse(200, "application/json", resp.dump());
            }

            if (method == "DELETE" && path.rfind("/vectors/", 0) == 0) {
                Collection* coll = getOrCreate(
                    queryParam(path_full, "collection").empty()
                        ? config_.default_collection
                        : queryParam(path_full, "collection"));
                uint64_t id = std::stoull(path.substr(9));
                coll->remove(id);
                json resp;
                resp["status"] = "ok";
                return buildResponse(200, "application/json", resp.dump());
            }

        } catch (const std::exception& e) {
            stats_.error_requests++;
            json resp;
            resp["error"] = e.what();
            return buildResponse(400, "application/json", resp.dump());
        }

        stats_.error_requests++;
        json err;
        err["error"] = "not found";
        return buildResponse(404, "application/json", err.dump());
    }

    FilterNode buildFilter(const json& f) {
        if (!f.contains("op")) return FilterNode::eq("", "");

        std::string op = f["op"];
        if (op == "eq") {
            return FilterNode::eq(f.value("field", ""), f.value("value", ""));
        }
        if (op == "contains") {
            return FilterNode::contains(f.value("field", ""), f.value("value", ""));
        }
        if (op == "gt") {
            return FilterNode::gt(f.value("field", ""), f.value("value", ""));
        }
        if (op == "lt") {
            return FilterNode::lt(f.value("field", ""), f.value("value", ""));
        }
        if (op == "and" && f.contains("children") && f["children"].is_array() &&
            f["children"].size() >= 2) {
            return FilterNode::andAlso(buildFilter(f["children"][0]), buildFilter(f["children"][1]));
        }
        if (op == "or" && f.contains("children") && f["children"].is_array() &&
            f["children"].size() >= 2) {
            return FilterNode::orElse(buildFilter(f["children"][0]), buildFilter(f["children"][1]));
        }
        return FilterNode::eq("", "");
    }

    DeepVectorServer* owner_;
    ServerConfig config_;
    ServerStats& stats_;
    int listen_fd_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    std::vector<int> client_fds_;
};

DeepVectorServer::DeepVectorServer(const ServerConfig& config, const CollectionConfig& coll_config)
    : config_(config) {
    registry_ = std::make_unique<CollectionRegistry>(config_.data_dir, coll_config);
    impl_ = std::make_unique<Impl>(this, config_, stats_);
}

DeepVectorServer::~DeepVectorServer() = default;

void DeepVectorServer::start() { impl_->start(); }
void DeepVectorServer::stop() { impl_->stop(); }

} // namespace server
} // namespace dv
