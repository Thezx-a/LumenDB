#include "skynet/net/tcp_stream.h"
#include "skynet/net/io_context.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace skynet {
namespace net {

TcpStream::TcpStream(std::unique_ptr<Socket> sock, IOContext* ctx)
    : sock_(std::move(sock)), ctx_(ctx) {}

bool TcpStream::readExact(void* buf, size_t len) {
    char* p = static_cast<char*>(buf);
    size_t total = 0;
    while (total < len) {
        ssize_t n = sock_->read(p + total, len - total);
        if (n <= 0) { if (n == 0) return false; if (errno == EAGAIN) continue; return false; }
        total += n;
    }
    return true;
}

bool TcpStream::readUntil(std::string& buf, const std::string& delim) {
    char tmp[4096];
    while (true) {
        auto pos = read_buf_.find(delim);
        if (pos != std::string::npos) {
            buf = read_buf_.substr(0, pos + delim.size());
            read_buf_.erase(0, pos + delim.size());
            return true;
        }
        ssize_t n = sock_->read(tmp, sizeof(tmp));
        if (n <= 0) { if (n == 0) return false; if (errno == EAGAIN) continue; return false; }
        read_buf_.append(tmp, n);
    }
}

bool TcpStream::writeAll(const void* buf, size_t len) {
    const char* p = static_cast<const char*>(buf);
    size_t total = 0;
    while (total < len) {
        ssize_t n = sock_->write(p + total, len - total);
        if (n <= 0) { if (errno == EAGAIN) continue; return false; }
        total += n;
    }
    return true;
}

void TcpStream::close() { sock_->close(); }

}  // namespace net
}  // namespace skynet
