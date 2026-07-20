#include "server.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

extern volatile sig_atomic_t g_running;

HttpServer::HttpServer(int port) : port_(port) {}

HttpServer::~HttpServer() {
    if (server_fd_ >= 0) close(server_fd_);
}

int HttpServer::create_listen_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(fd, 128) < 0) { perror("listen"); exit(1); }

    return fd;
}

HttpRequest HttpServer::parse_request(const std::string& raw) {
    HttpRequest req;
    size_t pos = raw.find("\r\n");
    if (pos == std::string::npos) return req;

    std::string line = raw.substr(0, pos);
    size_t p1 = line.find(' ');
    size_t p2 = line.find(' ', p1 + 1);
    if (p1 != std::string::npos && p2 != std::string::npos) {
        req.method = line.substr(0, p1);
        req.path = line.substr(p1 + 1, p2 - p1 - 1);
        req.version = line.substr(p2 + 1);
    }

    size_t hdr_end = raw.find("\r\n\r\n");
    if (hdr_end != std::string::npos) {
        std::string headers = raw.substr(pos + 2, hdr_end - pos - 2);
        size_t hpos = 0;
        while (hpos < headers.size()) {
            size_t hline = headers.find("\r\n", hpos);
            if (hline == std::string::npos) hline = headers.size();
            std::string hdr = headers.substr(hpos, hline - hpos);
            size_t colon = hdr.find(':');
            if (colon != std::string::npos) {
                std::string key = hdr.substr(0, colon);
                std::string val = hdr.substr(colon + 2);
                req.headers[key] = val;
            }
            hpos = hline + 2;
        }
        req.body = raw.substr(hdr_end + 4);
    }
    return req;
}

void HttpServer::handle_client(int client_fd) {
    char buf[65536];
    ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) { close(client_fd); return; }
    buf[n] = '\0';

    HttpRequest req = parse_request(buf);
    HttpResponse resp = router_.dispatch(req);

    std::string serialized = resp.serialize();
    send(client_fd, serialized.c_str(), serialized.size(), 0);
    close(client_fd);
}

void HttpServer::run() {
    server_fd_ = create_listen_socket();
    std::cout << "HTTP server listening on port " << port_ << "\n";

    std::vector<pollfd> fds;
    fds.push_back({server_fd_, POLLIN, 0});

    while (g_running) {
        int ret = poll(fds.data(), fds.size(), 1000);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }

        for (size_t i = 1; i < fds.size(); i++) {
            if (fds[i].revents & POLLIN) {
                handle_client(fds[i].fd);
                fds[i].fd = -1;
            }
        }

        fds.erase(
            std::remove_if(fds.begin() + 1, fds.end(),
                [](const pollfd& p) { return p.fd == -1; }),
            fds.end());

        if (fds[0].revents & POLLIN) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd_,
                (sockaddr*)&client_addr, &addr_len);
            if (client_fd >= 0) {
                fds.push_back({client_fd, POLLIN, 0});
            }
        }
    }

    std::cout << "Shutting down...\n";
    close(server_fd_);
    server_fd_ = -1;
}
