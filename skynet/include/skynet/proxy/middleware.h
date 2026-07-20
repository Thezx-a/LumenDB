#pragma once
#include <functional>
#include <vector>
#include "skynet/http/request.h"
#include "skynet/http/response.h"

namespace skynet {
namespace proxy {

using Middleware = std::function<http::HttpResponse(const http::HttpRequest&)>;

class MiddlewareChain {
public:
    void add(Middleware mw);
    http::HttpResponse process(const http::HttpRequest& req);

private:
    std::vector<Middleware> chain_;
};

}  // namespace proxy
}  // namespace skynet
