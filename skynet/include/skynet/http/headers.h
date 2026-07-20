#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace skynet {
namespace http {

class Headers {
public:
    void add(const std::string& name, const std::string& value);
    std::string get(const std::string& name) const;
    bool has(const std::string& name) const;
    void remove(const std::string& name);
    const std::vector<std::pair<std::string, std::string>>& all() const { return headers_; }

private:
    std::vector<std::pair<std::string, std::string>> headers_;
    std::unordered_map<std::string, std::string> lower_map_;
};

}  // namespace http
}  // namespace skynet
