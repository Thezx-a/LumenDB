#pragma once
#include <functional>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

namespace minikv {
namespace network {

class EventLoop {
public:
    using Callback = std::function<void(uint32_t)>;
    static const int kMaxEvents = 1024;

    EventLoop();
    ~EventLoop();

    void addEvent(int fd, uint32_t events, Callback callback);
    void updateEvent(int fd, uint32_t events);
    void removeEvent(int fd);
    void loop();
    void stop();

private:
    int epoll_fd_;
    bool running_;
    std::unordered_map<int, Callback> callbacks_;
};

}  // namespace network
}  // namespace minikv
