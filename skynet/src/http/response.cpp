#include "skynet/http/response.h"
#include <sstream>

namespace skynet {
namespace http {

std::string HttpResponse::serialize() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    for (const auto& [name, value] : headers.all()) {
        oss << name << ": " << value << "\r\n";
    }
    if (!headers.has("Content-Length")) {
        oss << "Content-Length: " << body.size() << "\r\n";
    }
    oss << "\r\n" << body;
    return oss.str();
}

HttpResponseBuilder& HttpResponseBuilder::status(int code, std::string text) {
    resp_.status_code = code;
    resp_.status_text = text.empty() ? (code == 200 ? "OK" : code == 404 ? "Not Found" : "Error") : text;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::header(const std::string& name, const std::string& value) {
    resp_.headers.add(name, value);
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::body(const std::string& b) {
    resp_.body = b;
    return *this;
}

HttpResponse HttpResponseBuilder::build() { return std::move(resp_); }

}  // namespace http
}  // namespace skynet
