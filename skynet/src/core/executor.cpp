#include "skynet/core/executor.h"
#include "skynet/core/timer_wheel.h"
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>
#include <chrono>

namespace skynet {
namespace core {

static const int kMaxEvents = 1024;

Executor::Executor() : running_(false) {
    epoll_fd_ = ::epoll_create1(0);
    timer_wheel_ = new TimerWheel(1024, 50);  // 50ms tick granularity
}

Executor::~Executor() {
    stop();
    if (epoll_fd_ >= 0) ::close(epoll_fd_);
    delete timer_wheel_;
}

void Executor::schedule(std::coroutine_handle<> coro) {
    ready_queue_.push_back(coro);
}

void Executor::addIO(int fd, uint32_t events, std::coroutine_handle<> coro) {
    struct epoll_event ev;
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    io_callbacks_[fd] = coro;
}

void Executor::removeIO(int fd) {
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    io_callbacks_.erase(fd);
}

void Executor::addTimer(uint64_t timeout_ms, std::coroutine_handle<> coro) {
    timer_wheel_->addTimer(timeout_ms, coro);
}

void Executor::run() {
    running_ = true;
    using Clock = std::chrono::steady_clock;
    auto last_time = Clock::now();

    while (running_) {
        // Process ready queue
        while (!ready_queue_.empty()) {
            auto coro = ready_queue_.front();
            ready_queue_.pop_front();
            if (!coro.done()) coro.resume();
        }

        // Get elapsed time for timer wheel
        auto now = Clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
        if (elapsed_ms > 0) timer_wheel_->advance(static_cast<uint64_t>(elapsed_ms));
        last_time = now;

        // Get expired timers
        auto expired = timer_wheel_->getExpired();
        for (auto& coro : expired) {
            if (!coro.done()) coro.resume();
        }

        // Process IO events
        processIO();
    }
}

void Executor::stop() { running_ = false; }

void Executor::processIO() {
    struct epoll_event events[kMaxEvents];
    int n = ::epoll_wait(epoll_fd_, events, kMaxEvents, 100);
    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        auto it = io_callbacks_.find(fd);
        if (it != io_callbacks_.end()) {
            schedule(it->second);
            io_callbacks_.erase(it);
        }
    }
}

}  // namespace core
}  // namespace skynet
