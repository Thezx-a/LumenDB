#include "skynet/net/socket.h"
#include "skynet/net/io_context.h"
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <iostream>

namespace skynet {
namespace net {

SocketAwaitable::SocketAwaitable(int fd, uint32_t events, IOContext* ctx)
    : fd_(fd), events_(events), ctx_(ctx), result_(-1) {}

void SocketAwaitable::await_suspend(std::coroutine_handle<> h) {
    handle_ = h;
    ctx_->add(fd_, events_, [this](uint32_t) {
        result_ = 0;
        handle_.resume();
    });
}

Socket::Socket(int fd) : fd_(fd) {}
Socket::~Socket() { if (fd_ >= 0) ::close(fd_); }

void Socket::setNonBlocking() {
    int flags = ::fcntl(fd_, F_GETFL, 0);
    ::fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
}

SocketAwaitable Socket::asyncRead(void* buf, size_t len, IOContext* ctx) {
    return SocketAwaitable(fd_, EPOLLIN, ctx);
}

SocketAwaitable Socket::asyncWrite(const void* buf, size_t len, IOContext* ctx) {
    return SocketAwaitable(fd_, EPOLLOUT, ctx);
}

ssize_t Socket::read(void* buf, size_t len) {
    return ::read(fd_, buf, len);
}

ssize_t Socket::write(const void* buf, size_t len) {
    return ::write(fd_, buf, len);
}

void Socket::close() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

}  // namespace net
}  // namespace skynet
