#include <chrono>
#include <iostream>
#include <string>
#include "skynet/http/parser.h"
using namespace skynet::http;

int main() {
    std::string req = "GET /hello?name=world HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n";
    int N = 1000000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        HttpParser parser;
        parser.feed(req.data(), req.size());
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "Parsed " << N << " requests in " << ms << "ms => "
              << (N * 1000.0 / ms) << " req/s" << std::endl;
}
