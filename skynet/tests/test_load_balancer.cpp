#include <gtest/gtest.h>
#include <unordered_map>
#include "skynet/proxy/upstream.h"
#include "skynet/proxy/load_balancer.h"
using namespace skynet::proxy;

TEST(WeightedRoundRobinTest, Distribution) {
    std::vector<Upstream> upstreams = {
        {"127.0.0.1", 8001, 5, {}}, {"127.0.0.1", 8002, 1, {}}, {"127.0.0.1", 8003, 1, {}}
    };
    UpstreamManager mgr;
    mgr.reload(upstreams);
    WeightedRoundRobinLB lb(&mgr);

    std::unordered_map<int, int> counts;
    for (int i = 0; i < 700; ++i) {
        auto up = lb.select();
        if (up) counts[up->port]++;
    }
    EXPECT_NEAR(counts[8001], 500, 10);
    EXPECT_NEAR(counts[8002], 100, 10);
    EXPECT_NEAR(counts[8003], 100, 10);
}

TEST(RoundRobinTest, Alternation) {
    std::vector<Upstream> upstreams = {
        {"10.0.0.1", 80, 1, {}}, {"10.0.0.2", 80, 1, {}}
    };
    UpstreamManager mgr;
    mgr.reload(upstreams);
    RoundRobinLB lb(&mgr);
    auto a = lb.select(); auto b = lb.select(); auto c = lb.select();
    ASSERT_TRUE(a && b && c);
    EXPECT_NE(a->host, b->host);
    EXPECT_EQ(a->host, c->host);
}
