#include "skynet/http/router.h"
#include <unordered_map>

namespace skynet {
namespace http {

void Router::route(const std::string& method, const std::string& pattern, Handler h) {
    routes_.push_back({method, pattern, std::move(h)});
}

HttpResponse Router::dispatch(const HttpRequest& req) {
    for (const auto& r : routes_) {
        if (r.method == req.method) {
            std::unordered_map<std::string, std::string> params;
            if (match(r.pattern, req.path, params)) {
                return r.handler(req);
            }
        }
    }
    HttpResponse resp;
    resp.status_code = 404;
    resp.status_text = "Not Found";
    resp.body = "Not Found";
    return resp;
}

bool Router::match(const std::string& pattern, const std::string& path,
                   std::unordered_map<std::string, std::string>& params) {
    if (pattern == path) return true;
    // Simple :param matching
    size_t pi = 0, si = 0;
    while (pi < pattern.size() && si < path.size()) {
        if (pattern[pi] == ':') {
            size_t nextSlash = pattern.find('/', pi);
            std::string paramName = pattern.substr(pi + 1, nextSlash - pi - 1);
            size_t pathSlash = path.find('/', si);
            std::string paramValue = path.substr(si, pathSlash - si);
            params[paramName] = paramValue;
            pi = nextSlash; si = pathSlash;
        } else if (pattern[pi] != path[si]) {
            return false;
        } else {
            ++pi; ++si;
        }
    }
    return pi >= pattern.size() && si >= path.size();
}

}  // namespace http
}  // namespace skynet
