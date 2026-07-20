#pragma once
#include <functional>
#include <string>
#include "minikv/status.h"

namespace minikv {
namespace network {

class Connection {
public:
    using RequestHandler = std::function<std::string(const std::string&)>;

    explicit Connection(int fd, RequestHandler handler);
    ~Connection();

    void onReadable();
    void onWritable();
    int fd() const { return fd_; }
    bool shouldClose() const { return close_; }

private:
    int fd_;
    RequestHandler handler_;
    std::string read_buf_;
    std::string write_buf_;
    bool close_;
};

}  // namespace network
}  // namespace minikv
