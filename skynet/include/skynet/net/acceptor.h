#pragma once
#include <coroutine>
#include <memory>
#include <string>
#include "skynet/net/socket.h"

namespace skynet {
namespace net {

class IOContext;

class AcceptAwaitable {
public:
    AcceptAwaitable(int listen_fd, IOContext* ctx);
    bool await_ready() const;
    void await_suspend(std::coroutine_handle<> h);
    std::unique_ptr<Socket> await_resume();

private:
    int listen_fd_;
    IOContext* ctx_;
    mutable int client_fd_;
    std::coroutine_handle<> handle_;
};

class Acceptor {
public:
    Acceptor(const std::string& host, int port);
    ~Acceptor();
    bool bindAndListen();
    AcceptAwaitable accept(IOContext* ctx);
    int fd() const { return listen_fd_; }

private:
    std::string host_;
    int port_;
    int listen_fd_;
};

}  // namespace net
}  // namespace skynet
