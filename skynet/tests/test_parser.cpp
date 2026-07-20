#include <gtest/gtest.h>
#include "skynet/http/parser.h"
using namespace skynet::http;

TEST(ParserTest, SimpleGet) {
    HttpParser parser;
    std::string req = "GET /hello?name=world HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto r = parser.feed(req.data(), req.size());
    EXPECT_EQ(r, ParseResult::kOk);
    EXPECT_TRUE(parser.isComplete());
    auto result = parser.result();
    EXPECT_EQ(result.method, "GET");
    EXPECT_EQ(result.path, "/hello");
    EXPECT_EQ(result.query, "name=world");
    EXPECT_EQ(result.headers.get("Host"), "localhost");
}

TEST(ParserTest, PostWithBody) {
    HttpParser parser;
    std::string req = "POST /api HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello";
    auto r = parser.feed(req.data(), req.size());
    EXPECT_EQ(r, ParseResult::kOk);
    auto result = parser.result();
    EXPECT_EQ(result.method, "POST");
    EXPECT_EQ(result.body, "hello");
}

TEST(ParserTest, PartialFeed) {
    HttpParser parser;
    std::string part1 = "GET /test HTTP/1.1\r\nHost: ";
    std::string part2 = "localhost\r\n\r\n";
    auto r1 = parser.feed(part1.data(), part1.size());
    EXPECT_EQ(r1, ParseResult::kNeedMoreData);
    auto r2 = parser.feed(part2.data(), part2.size());
    EXPECT_EQ(r2, ParseResult::kOk);
    EXPECT_TRUE(parser.isComplete());
}
