#include "skynet/net/resolver.h"
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace skynet {
namespace net {

Resolver::Resolver(core::ThreadPool* pool) : pool_(pool) {}

std::vector<std::string> Resolver::resolve(const std::string& hostname) {
    std::vector<std::string> result;
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (::getaddrinfo(hostname.c_str(), nullptr, &hints, &res) == 0) {
        for (auto* p = res; p; p = p->ai_next) {
            char ip[INET_ADDRSTRLEN];
            auto* sin = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            ::inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
            result.emplace_back(ip);
        }
        ::freeaddrinfo(res);
    }
    return result;
}

}  // namespace net
}  // namespace skynet
