#include "lumendb/server/server.h"
#include "lumendb/collection.h"
#include "lumendb/filter.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <deque>
#include <thread>

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

static std::string bodyFromRequest(const std::string& raw) {
    auto pos = raw.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return raw.substr(pos + 4);
}

class DeepVectorServer::Impl {
public:
    Impl(const ServerConfig& cfg, Collection* coll, ServerStats& stats)
        : config_(cfg), collection_(coll), stats_(stats), listen_fd_(-1), running_(false) {}

    ~Impl() { stop(); }

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
            std::cerr << "DeepVector: failed to bind to " << config_.host << ":" << config_.port << std::endl;
            ::close(listen_fd_);
            listen_fd_ = -1;
            return;
        }
        ::listen(listen_fd_, 128);

        running_ = true;
        std::cout << "DeepVector server listening on " << config_.host << ":" << config_.port << std::endl;

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
                int client = ::accept(listen_fd_, nullptr, nullptr);
                if (client >= 0) {
                    setNonBlocking(client);
                    clients.push_back(client);
                    stats_.active_connections++;
                }
            }

            for (auto it = clients.begin(); it != clients.end();) {
                int fd = *it;
                if (FD_ISSET(fd, &readfds)) {
                    ssize_t nread = ::read(fd, buf, sizeof(buf) - 1);
                    if (nread <= 0) {
                        ::close(fd);
                        stats_.active_connections--;
                        it = clients.erase(it);
                        continue;
                    }
                    buf[nread] = '\0';
                    std::string request(buf, nread);
                    std::string response = handleRequest(request);
                    ::write(fd, response.data(), response.size());
                }
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
        std::string path = pathFromRequest(raw);
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
            // --- AgenticDB Enhanced Endpoints ---

            if (path == "/collections" && method == "GET") {
                json resp;
                json coll;
                coll["name"] = "default";
                coll["vectors"] = collection_->size();
                coll["dim"] = collection_->dim();
                resp["collections"] = json::array();
                resp["collections"].push_back(coll);
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/batch/search" && method == "POST") {
                stats_.search_requests++;
                auto req = json::parse(body);
                json resp;
                resp["results"] = json::array();

                for (auto& q : req["queries"]) {
                    std::vector<float> vec = q["vector"].get<std::vector<float>>();
                    size_t k = q.value("k", 10);
                    auto results = collection_->search(vec.data(), k);

                    json batch_results;
                    batch_results["results"] = json::array();
                    for (auto& r : results) {
                        json item;
                        item["id"] = r.id;
                        item["distance"] = r.distance;
                        batch_results["results"].push_back(item);
                    }
                    resp["results"].push_back(batch_results);
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/vector" && method == "GET") {
                // GET /vector?id=123 - get a specific vector
                auto pos = path.find("?");
                if (pos != std::string::npos) {
                    auto query = path.substr(pos + 1);
                    auto id_pos = query.find("id=");
                    if (id_pos != std::string::npos) {
                        uint64_t id = std::stoull(query.substr(id_pos + 3));
                        const float* vec = collection_->getVector(id);
                        if (vec) {
                            json resp;
                            resp["id"] = id;
                            resp["vector"] = json::array();
                            for (Dimension d = 0; d < collection_->dim(); d++) {
                                resp["vector"].push_back(vec[d]);
                            }
                            return buildResponse(200, "application/json", resp.dump());
                        }
                    }
                }
                json err;
                err["error"] = "vector not found";
                return buildResponse(404, "application/json", err.dump());
            }

            // --- Original Endpoints ---

            if (path == "/health" && method == "GET") {
                json resp;
                resp["status"] = "ok";
                resp["vectors"] = collection_->size();
                resp["dim"] = collection_->dim();
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

            if (path == "/search" && method == "POST") {
                stats_.search_requests++;
                auto req = json::parse(body);
                std::vector<float> vec = req["vector"].get<std::vector<float>>();
                size_t k = req.value("k", 10);

                std::vector<SearchResult> results;
                if (req.contains("filter")) {
                    auto f = req["filter"];
                    FilterNode filterNode = buildFilter(f);
                    results = collection_->searchWithFilter(vec.data(), k, filterNode);
                } else {
                    results = collection_->search(vec.data(), k);
                }

                json resp;
                resp["results"] = json::array();
                for (auto& r : results) {
                    json item;
                    item["id"] = r.id;
                    item["distance"] = r.distance;
                    resp["results"].push_back(item);
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            if (path == "/insert" && method == "POST") {
                stats_.insert_requests++;
                auto req = json::parse(body);
                json resp;
                resp["ids"] = json::array();

                if (req.contains("vectors")) {
                    for (auto& v : req["vectors"]) {
                        std::vector<float> vec = v.get<std::vector<float>>();
                        uint64_t id = collection_->add(vec.data());
                        resp["ids"].push_back(id);
                    }
                } else if (req.contains("vector")) {
                    std::vector<float> vec = req["vector"].get<std::vector<float>>();
                    uint64_t id = collection_->add(vec.data());
                    resp["ids"].push_back(id);
                }
                return buildResponse(200, "application/json", resp.dump());
            }

            if (method == "DELETE" && path.find("/vectors/") == 0) {
                uint64_t id = std::stoull(path.substr(9));
                collection_->remove(id);
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
        if (op == "and" && f.contains("children") && f["children"].is_array() && f["children"].size() >= 2) {
            return FilterNode::andAlso(buildFilter(f["children"][0]), buildFilter(f["children"][1]));
        }
        if (op == "or" && f.contains("children") && f["children"].is_array() && f["children"].size() >= 2) {
            return FilterNode::orElse(buildFilter(f["children"][0]), buildFilter(f["children"][1]));
        }
        return FilterNode::eq("", "");
    }

    ServerConfig config_;
    Collection* collection_;
    ServerStats& stats_;
    int listen_fd_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    std::vector<int> client_fds_;
};

DeepVectorServer::DeepVectorServer(const ServerConfig& config, std::unique_ptr<Collection> collection)
    : config_(config), collection_(std::move(collection)) {
    impl_ = std::make_unique<Impl>(config_, collection_.get(), stats_);
}

DeepVectorServer::~DeepVectorServer() = default;

void DeepVectorServer::start() { impl_->start(); }
void DeepVectorServer::stop() { impl_->stop(); }

} // namespace server
} // namespace dv
