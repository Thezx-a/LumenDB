#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "skynet/http/request.h"
#include "skynet/http/response.h"

namespace skynet {
namespace http {

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void route(const std::string& method, const std::string& pattern, Handler h);
    HttpResponse dispatch(const HttpRequest& req);

private:
    struct Route {
        std::string method;
        std::string pattern;
        Handler handler;
    };
    std::vector<Route> routes_;
    bool match(const std::string& pattern, const std::string& path,
               std::unordered_map<std::string, std::string>& params);
};

}  // namespace http
}  // namespace skynet
