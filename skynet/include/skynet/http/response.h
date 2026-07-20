#pragma once
#include <string>
#include "skynet/http/headers.h"

namespace skynet {
namespace http {

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    Headers headers;
    std::string body;
    std::string serialize() const;
};

class HttpResponseBuilder {
public:
    HttpResponseBuilder& status(int code, std::string text = "");
    HttpResponseBuilder& header(const std::string& name, const std::string& value);
    HttpResponseBuilder& body(const std::string& b);
    HttpResponse build();

private:
    HttpResponse resp_;
};

}  // namespace http
}  // namespace skynet
