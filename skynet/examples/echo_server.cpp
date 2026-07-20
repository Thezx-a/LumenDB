#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(9999); addr.sin_addr.s_addr = INADDR_ANY;
    ::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ::listen(listen_fd, 128);
    std::cout << "Echo server on :9999" << std::endl;
    while (true) {
        int fd = ::accept(listen_fd, nullptr, nullptr);
        char buf[4096];
        ssize_t n;
        while ((n = ::read(fd, buf, sizeof(buf))) > 0) ::write(fd, buf, n);
        ::close(fd);
    }
}
