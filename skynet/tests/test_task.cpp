#include <gtest/gtest.h>
#include "skynet/core/task.h"
using namespace skynet::core;

TEST(TaskTest, SimpleReturn) {
    auto inner = []() -> Task<int> { co_return 42; };
    auto outer = [&]() -> Task<int> { co_return co_await inner(); };
    auto task = outer();
    task.handle().resume();
    EXPECT_TRUE(task.handle().done());
    EXPECT_EQ(*task.handle().promise().result_, 42);
}

TEST(TaskVoidTest, SimpleVoid) {
    auto coro = []() -> Task<void> { co_return; };
    SUCCEED();
}
