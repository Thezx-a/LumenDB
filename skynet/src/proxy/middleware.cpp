#include "skynet/proxy/middleware.h"
namespace skynet {
namespace proxy {

void MiddlewareChain::add(Middleware mw) { chain_.push_back(std::move(mw)); }

http::HttpResponse MiddlewareChain::process(const http::HttpRequest& req) {
    for (auto& mw : chain_) {
        auto resp = mw(req);
        if (resp.status_code >= 400) return resp;
    }
    http::HttpResponse resp;
    resp.status_code = 200;
    return resp;
}

}  // namespace proxy
}  // namespace skynet
