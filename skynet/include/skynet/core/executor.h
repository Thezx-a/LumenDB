#pragma once
#include <coroutine>
#include <cstdint>
#include <cstddef>
#include <deque>
#include <functional>
#include <unordered_map>

namespace skynet {
namespace core {

class Executor {
public:
    using CoroCallback = std::function<void(std::coroutine_handle<>)>;

    Executor();
    ~Executor();

    void schedule(std::coroutine_handle<> coro);
    void addIO(int fd, uint32_t events, std::coroutine_handle<> coro);
    void removeIO(int fd);
    void addTimer(uint64_t timeout_ms, std::coroutine_handle<> coro);
    void run();
    void stop();

private:
    int epoll_fd_;
    std::deque<std::coroutine_handle<>> ready_queue_;
    bool running_;
    std::unordered_map<int, std::coroutine_handle<>> io_callbacks_;
    class TimerWheel* timer_wheel_;

    void processIO();
    void processTimers();
};

}  // namespace core
}  // namespace skynet
