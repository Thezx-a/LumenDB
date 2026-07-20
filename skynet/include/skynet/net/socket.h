#pragma once
#include <cstddef>
#include <coroutine>
#include <cstdint>
#include <cstring>
#include <string>
#include <sys/socket.h>

namespace skynet {
namespace net {

class IOContext;

class SocketAwaitable {
public:
    SocketAwaitable(int fd, uint32_t events, class IOContext* ctx);
    bool await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> h);
    int await_resume() const { return result_; }

private:
    int fd_;
    uint32_t events_;
    IOContext* ctx_;
    int result_;
    std::coroutine_handle<> handle_;
};

class Socket {
public:
    explicit Socket(int fd);
    ~Socket();
    int fd() const { return fd_; }
    void setNonBlocking();
    SocketAwaitable asyncRead(void* buf, size_t len, IOContext* ctx);
    SocketAwaitable asyncWrite(const void* buf, size_t len, IOContext* ctx);
    ssize_t read(void* buf, size_t len);
    ssize_t write(const void* buf, size_t len);
    void close();

private:
    int fd_;
};

}  // namespace net
}  // namespace skynet
