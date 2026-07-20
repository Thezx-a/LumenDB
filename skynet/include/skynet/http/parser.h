#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "skynet/http/request.h"

namespace skynet {
namespace http {

enum class ParseResult { kOk, kNeedMoreData, kError };

class HttpParser {
public:
    enum class State {
        kMethod, kPath, kVersion, kHeaderName, kHeaderValue, kBody, kDone
    };

    HttpParser();
    ParseResult feed(const char* data, size_t len);
    bool isComplete() const { return state_ == State::kDone; }
    HttpRequest result() const { return std::move(req_); }
    void reset();

private:
    State state_;
    HttpRequest req_;
    std::string buffer_;
    size_t content_length_;
    bool chunked_;
    std::string current_header_name_;
};

}  // namespace http
}  // namespace skynet
