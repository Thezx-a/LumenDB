#pragma once
#include <string>
#include "skynet/http/headers.h"

namespace skynet {
namespace http {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string version;
    Headers headers;
    std::string body;
};

}  // namespace http
}  // namespace skynet
