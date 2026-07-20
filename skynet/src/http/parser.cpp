#include "skynet/http/parser.h"
#include <algorithm>
#include <cctype>

namespace skynet {
namespace http {

HttpParser::HttpParser()
    : state_(State::kMethod), content_length_(0), chunked_(false) {}

void HttpParser::reset() {
    state_ = State::kMethod;
    req_ = HttpRequest{};
    buffer_.clear();
    content_length_ = 0;
    chunked_ = false;
    current_header_name_.clear();
}

ParseResult HttpParser::feed(const char* data, size_t len) {
    buffer_.append(data, len);
    size_t pos = 0;
    while (pos < buffer_.size()) {
        if (state_ == State::kBody) {
            if (buffer_.size() - pos >= content_length_) {
                req_.body = buffer_.substr(pos, content_length_);
                state_ = State::kDone;
                return ParseResult::kOk;
            } else {
                buffer_.erase(0, pos);
                return ParseResult::kNeedMoreData;
            }
        }
        size_t lineEnd = buffer_.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            buffer_.erase(0, pos);
            return ParseResult::kNeedMoreData;
        }
        std::string line = buffer_.substr(pos, lineEnd - pos);
        pos = lineEnd + 2;

        if (state_ == State::kMethod) {
            size_t sp1 = line.find(' ');
            size_t sp2 = line.find(' ', sp1 + 1);
            req_.method = line.substr(0, sp1);
            std::string fullPath = line.substr(sp1 + 1, sp2 - sp1 - 1);
            req_.version = line.substr(sp2 + 1);
            size_t qPos = fullPath.find('?');
            if (qPos != std::string::npos) {
                req_.path = fullPath.substr(0, qPos);
                req_.query = fullPath.substr(qPos + 1);
            } else {
                req_.path = fullPath;
            }
            state_ = State::kHeaderName;
        } else if (state_ == State::kHeaderName) {
            if (line.empty()) {
                if (content_length_ > 0 || chunked_) {
                    state_ = State::kBody;
                } else {
                    state_ = State::kDone;
                    return ParseResult::kOk;
                }
            } else {
                size_t colon = line.find(':');
                if (colon == std::string::npos) return ParseResult::kError;
                std::string name = line.substr(0, colon);
                std::string value = line.substr(colon + 2);
                req_.headers.add(name, value);
                if (name == "Content-Length") content_length_ = std::stoul(value);
            }
        }
    }
    buffer_.erase(0, pos);
    return state_ == State::kDone ? ParseResult::kOk : ParseResult::kNeedMoreData;
}

}  // namespace http
}  // namespace skynet
