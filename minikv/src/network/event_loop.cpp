#include "network/event_loop.h"
#include <unistd.h>
#include <iostream>

namespace minikv {
namespace network {

EventLoop::EventLoop() : running_(false) {
    epoll_fd_ = ::epoll_create1(0);
    if (epoll_fd_ < 0) {
        std::cerr << "epoll_create1 failed" << std::endl;
    }
}

EventLoop::~EventLoop() {
    if (epoll_fd_ >= 0) ::close(epoll_fd_);
}

void EventLoop::addEvent(int fd, uint32_t events, Callback callback) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    callbacks_[fd] = std::move(callback);
}

void EventLoop::updateEvent(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

void EventLoop::removeEvent(int fd) {
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    callbacks_.erase(fd);
}

void EventLoop::loop() {
    running_ = true;
    struct epoll_event events[kMaxEvents];
    while (running_) {
        int n = ::epoll_wait(epoll_fd_, events, kMaxEvents, -1);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            auto it = callbacks_.find(fd);
            if (it != callbacks_.end()) {
                it->second(events[i].events);
            }
        }
    }
}

void EventLoop::stop() { running_ = false; }

}  // namespace network
}  // namespace minikv
