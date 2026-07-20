#include <gtest/gtest.h>
#include "minikv/slice.h"
using minikv::Slice;

TEST(SliceTest, DefaultConstruct) {
    Slice s;
    EXPECT_EQ(s.size(), 0u);
    EXPECT_TRUE(s.empty());
}

TEST(SliceTest, FromString) {
    std::string str = "hello";
    Slice s(str);
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(s.toString(), "hello");
}

TEST(SliceTest, Compare) {
    Slice a("aaa"), b("bbb"), c("aaa");
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(b > a);
}

TEST(SliceTest, StartsWith) {
    Slice s("hello world");
    EXPECT_TRUE(s.startsWith(Slice("hello")));
    EXPECT_FALSE(s.startsWith(Slice("world")));
}
