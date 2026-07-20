#include "skynet/http/headers.h"
#include <cctype>
#include <algorithm>

namespace skynet {
namespace http {

static std::string toLower(const std::string& s) {
    std::string r;
    for (char c : s) r += static_cast<char>(::tolower(c));
    return r;
}

void Headers::add(const std::string& name, const std::string& value) {
    headers_.push_back({name, value});
    lower_map_[toLower(name)] = value;
}

std::string Headers::get(const std::string& name) const {
    auto it = lower_map_.find(toLower(name));
    return it != lower_map_.end() ? it->second : "";
}

bool Headers::has(const std::string& name) const {
    auto it = lower_map_.find(toLower(name));
    return it != lower_map_.end();
}

void Headers::remove(const std::string& name) {
    auto key = toLower(name);
    lower_map_.erase(key);
    headers_.erase(std::remove_if(headers_.begin(), headers_.end(),
        [&](const auto& p) { return toLower(p.first) == key; }), headers_.end());
}

}  // namespace http
}  // namespace skynet
