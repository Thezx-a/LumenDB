#include <gtest/gtest.h>
#include "skynet/http/router.h"
using namespace skynet::http;

TEST(RouterTest, ExactMatch) {
    Router r;
    r.route("GET", "/hello", [](const HttpRequest&) {
        return HttpResponseBuilder().status(200).body("hi").build();
    });
    HttpRequest req; req.method = "GET"; req.path = "/hello";
    auto resp = r.dispatch(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_EQ(resp.body, "hi");
}

TEST(RouterTest, NotFound) {
    Router r;
    HttpRequest req; req.method = "GET"; req.path = "/nonexistent";
    auto resp = r.dispatch(req);
    EXPECT_EQ(resp.status_code, 404);
}
