#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "skynet/http/parser.h"
#include "skynet/http/response.h"
#include "skynet/http/router.h"

int main() {
    using namespace skynet::http;
    Router router;
    router.route("GET", "/hello", [](const HttpRequest&) {
        return HttpResponseBuilder().status(200).body("Hello from SkyNet!").build();
    });
    router.route("GET", "/", [](const HttpRequest&) {
        return HttpResponseBuilder().status(200).body("SkyNet HTTP Server").build();
    });

    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(8080); addr.sin_addr.s_addr = INADDR_ANY;
    ::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ::listen(listen_fd, 128);
    std::cout << "HTTP server on :8080" << std::endl;

    while (true) {
        int fd = ::accept(listen_fd, nullptr, nullptr);
        char buf[8192];
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n > 0) {
            HttpParser parser;
            if (parser.feed(buf, n) == ParseResult::kOk) {
                auto resp = router.dispatch(parser.result());
                auto raw = resp.serialize();
                ::write(fd, raw.data(), raw.size());
            }
        }
        ::close(fd);
    }
}
