#include "server.h"

#include <csignal>
#include <iostream>

volatile sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    HttpServer server(8080);

    server.router().get("/health", [](HttpRequest&) {
        return HttpResponse(200, "OK",
            R"({"status":"ok","service":"deepvector"})");
    });

    server.router().get("/api/v1/stats", [](HttpRequest&) {
        return HttpResponse(200, "OK",
            R"({"vectors":0,"dimension":128,"status":"healthy"})");
    });

    server.router().post("/api/v1/echo", [](HttpRequest& req) {
        return HttpResponse(200, "OK", req.body);
    });

    server.run();
    return 0;
}
