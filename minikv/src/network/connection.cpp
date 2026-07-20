#include "network/connection.h"
#include <unistd.h>
#include <cstring>
#include "network/protocol.h"

namespace minikv {
namespace network {

Connection::Connection(int fd, RequestHandler handler)
    : fd_(fd), handler_(std::move(handler)), close_(false) {}

Connection::~Connection() {
    if (fd_ >= 0) ::close(fd_);
}

void Connection::onReadable() {
    char buf[4096];
    ssize_t n = ::read(fd_, buf, sizeof(buf));
    if (n <= 0) { close_ = true; return; }
    read_buf_.append(buf, n);

    while (read_buf_.size() >= sizeof(RequestHeader)) {
        auto* hdr = reinterpret_cast<const RequestHeader*>(read_buf_.data());
        if (hdr->magic != kProtocolMagic) { close_ = true; return; }
        size_t totalSize = sizeof(RequestHeader) + hdr->key_len + hdr->val_len;
        if (read_buf_.size() < totalSize) break;

        // Build raw request string
        std::string rawData = read_buf_.substr(0, totalSize);
        read_buf_.erase(0, totalSize);

        // Call handler and write response
        std::string response = handler_(rawData);
        write_buf_.append(response);
    }
    if (!write_buf_.empty()) onWritable();
}

void Connection::onWritable() {
    if (write_buf_.empty()) return;
    ssize_t n = ::write(fd_, write_buf_.data(), write_buf_.size());
    if (n > 0) { write_buf_.erase(0, n); }
}

}  // namespace network
}  // namespace minikv
