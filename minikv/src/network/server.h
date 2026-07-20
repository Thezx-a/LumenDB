#pragma once
#include <memory>
#include <string>
#include "network/event_loop.h"
#include "minikv/db.h"

namespace minikv {
namespace network {

class Server {
public:
    Server(const std::string& host, int port, ::minikv::DB* db);
    ~Server();
    void run();
    void stop();

private:
    void handleNewConnection();
    std::string processRequest(const std::string& rawData);

    std::string host_;
    int port_;
    ::minikv::DB* db_;
    int listen_fd_;
    EventLoop loop_;
};

}  // namespace network
}  // namespace minikv
