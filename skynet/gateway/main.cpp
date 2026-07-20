#include <iostream>
#include <csignal>
#include <memory>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "skynet/proxy/upstream.h"
#include "skynet/proxy/load_balancer.h"
#include "skynet/proxy/health_check.h"
#include "skynet/proxy/connection_pool.h"
#include "skynet/http/parser.h"
#include "skynet/http/response.h"
#include "skynet/config/config.h"

static volatile bool g_running = true;
void signalHandler(int) { g_running = false; }

void handle_client(int client_fd, skynet::proxy::LoadBalancer* lb,
                   skynet::proxy::ConnectionPool* pool) {
    char buf[8192];
    ssize_t n = ::read(client_fd, buf, sizeof(buf));
    if (n <= 0) { ::close(client_fd); return; }

    skynet::http::HttpParser parser;
    auto result = parser.feed(buf, n);
    if (result != skynet::http::ParseResult::kOk) {
        ::close(client_fd);
        return;
    }
    auto req = parser.result();
    auto upstream = lb->select();
    if (!upstream) {
        std::string resp = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n";
        ::write(client_fd, resp.data(), resp.size());
        ::close(client_fd);
        return;
    }

    upstream->active_connections++;
    auto backendSock = pool->acquire(*upstream);
    if (!backendSock) {
        std::string resp = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        ::write(client_fd, resp.data(), resp.size());
        ::close(client_fd);
        return;
    }

    std::string rawReq = std::string(buf, n);
    backendSock->write(rawReq.data(), rawReq.size());

    char backendBuf[8192];
    ssize_t bn = backendSock->read(backendBuf, sizeof(backendBuf));
    if (bn > 0) {
        ::write(client_fd, backendBuf, bn);
    }
    pool->release(std::move(backendSock), *upstream);
    upstream->active_connections--;
    ::close(client_fd);
}

int main(int argc, char* argv[]) {
    std::string configPath = "gateway.yaml";
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--config" && i + 1 < argc) configPath = argv[++i];
    }
    auto cfg = skynet::config::Config::load(configPath);
    if (!cfg) { std::cerr << "Failed to load config: " << configPath << std::endl; return 1; }

    ::signal(SIGINT, signalHandler);
    ::signal(SIGTERM, signalHandler);

    std::vector<skynet::proxy::Upstream> upstreams;
    for (const auto& u : cfg->upstreams) {
        skynet::proxy::Upstream up;
        up.host = u.host; up.port = u.port; up.weight = u.weight;
        upstreams.push_back(up);
    }

    skynet::proxy::UpstreamManager upstreamMgr;
    upstreamMgr.reload(upstreams);
    skynet::proxy::WeightedRoundRobinLB lb(&upstreamMgr);
    skynet::proxy::ConnectionPool pool(10);
    skynet::proxy::HealthCheck hc(&upstreamMgr, cfg->health_check.interval_s,
                                   cfg->health_check.timeout_ms);
    hc.start();

    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg->listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Bind failed on port " << cfg->listen_port << std::endl; return 1;
    }
    ::listen(listen_fd, 128);
    std::cout << "SkyNet gateway listening on port " << cfg->listen_port << std::endl;

    while (g_running) {
        int client_fd = ::accept(listen_fd, nullptr, nullptr);
        if (client_fd < 0) continue;
        handle_client(client_fd, &lb, &pool);
    }
    hc.stop();
    ::close(listen_fd);
    return 0;
}
