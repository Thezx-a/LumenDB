#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    HttpResponse() = default;
    HttpResponse(int code, const std::string& text,
                 const std::string& body = "")
        : status_code(code), status_text(text), body(body) {
        headers["Content-Type"] = "application/json";
    }

    std::string serialize() const {
        std::string resp = "HTTP/1.1 " + std::to_string(status_code)
                         + " " + status_text + "\r\n";
        resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        for (auto& [k, v] : headers) {
            resp += k + ": " + v + "\r\n";
        }
        resp += "\r\n";
        resp += body;
        return resp;
    }
};

class Router {
    std::unordered_map<std::string,
        std::function<HttpResponse(HttpRequest&)>> routes_;

public:
    void get(const std::string& path,
             std::function<HttpResponse(HttpRequest&)> handler) {
        routes_["GET " + path] = handler;
    }
    void post(const std::string& path,
              std::function<HttpResponse(HttpRequest&)> handler) {
        routes_["POST " + path] = handler;
    }

    HttpResponse dispatch(HttpRequest& req) {
        std::string key = req.method + " " + req.path;
        auto it = routes_.find(key);
        if (it != routes_.end()) {
            return it->second(req);
        }
        return HttpResponse(404, "Not Found",
            R"({"status":"error","error":{"code":"NOT_FOUND"}})");
    }
};

class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();
    Router& router() { return router_; }
    void run();

private:
    int create_listen_socket();
    HttpRequest parse_request(const std::string& raw);
    void handle_client(int client_fd);

    int port_;
    int server_fd_ = -1;
    Router router_;
};
