#include "network/server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <memory>
#include "network/connection.h"
#include "network/protocol.h"

namespace minikv {
namespace network {

Server::Server(const std::string& host, int port, ::minikv::DB* db)
    : host_(host), port_(port), db_(db), listen_fd_(-1) {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    ::inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Bind failed on port " << port_ << std::endl;
        return;
    }
    ::listen(listen_fd_, 128);
    std::cout << "MiniKV server listening on " << host_ << ":" << port_ << std::endl;
}

Server::~Server() {
    if (listen_fd_ >= 0) ::close(listen_fd_);
}

void Server::run() {
    loop_.addEvent(listen_fd_, EPOLLIN, [this](uint32_t) { handleNewConnection(); });
    loop_.loop();
}

void Server::stop() { loop_.stop(); }

void Server::handleNewConnection() {
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);
    int connFd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
    if (connFd < 0) return;

    auto* conn = new Connection(connFd, [this](const std::string& raw) {
        return processRequest(raw);
    });

    loop_.addEvent(connFd, EPOLLIN, [this, conn, connFd](uint32_t events) {
        conn->onReadable();
        if (conn->shouldClose()) {
            loop_.removeEvent(connFd);
            delete conn;
        }
    });
}

std::string Server::processRequest(const std::string& rawData) {
    if (rawData.size() < sizeof(RequestHeader)) {
        return encodeResponse(ResponseStatus::kError, Slice());
    }
    auto* hdr = reinterpret_cast<const RequestHeader*>(rawData.data());
    const char* key = rawData.data() + sizeof(RequestHeader);
    const char* val = key + hdr->key_len;
    size_t valLen = hdr->val_len;

    auto cmd = static_cast<Cmd>(hdr->cmd);
    WriteOptions wopts;
    ReadOptions ropts;

    switch (cmd) {
        case Cmd::kPut: {
            db_->put(wopts, Slice(key, hdr->key_len), Slice(val, valLen));
return encodeResponse(ResponseStatus::kOk, Slice());
        }
        case Cmd::kGet: {
            std::string value;
            Status s = db_->get(ropts, Slice(key, hdr->key_len), &value);
            if (s.ok()) return encodeResponse(ResponseStatus::kOk, value);
            return encodeResponse(ResponseStatus::kNotFound, Slice());
        }
        case Cmd::kDel: {
            db_->del(wopts, Slice(key, hdr->key_len));
            return encodeResponse(ResponseStatus::kOk, Slice());
        }
        default:
            return encodeResponse(ResponseStatus::kError, Slice("Unknown command", 15));
    }
}

}  // namespace network
}  // namespace minikv
