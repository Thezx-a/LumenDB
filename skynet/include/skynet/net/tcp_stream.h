#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include "skynet/net/socket.h"

namespace skynet {
namespace net {

class IOContext;

class TcpStream {
public:
    TcpStream(std::unique_ptr<Socket> sock, IOContext* ctx);
    bool readExact(void* buf, size_t len);
    bool readUntil(std::string& buf, const std::string& delim);
    bool writeAll(const void* buf, size_t len);
    Socket& socket() { return *sock_; }
    void close();

private:
    std::unique_ptr<Socket> sock_;
    IOContext* ctx_;
    std::string read_buf_;
};

}  // namespace net
}  // namespace skynet
