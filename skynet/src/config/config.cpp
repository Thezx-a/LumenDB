#include "skynet/config/config.h"
#include <fstream>
#include <iostream>
#include <memory>

namespace skynet {
namespace config {

// Simplified YAML parser - handles our specific format with basic key-value parsing
std::unique_ptr<Config> Config::load(const std::string& path) {
    auto cfg = std::make_unique<Config>();
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Cannot open config: " << path << std::endl;
        return nullptr;
    }
    std::string line;
    std::string section;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (line.find(':') != std::string::npos && line[line.size()-1] == ':') {
            section = line.substr(0, line.size() - 1);
            continue;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        size_t s = val.find_first_not_of(" \t");
        if (s != std::string::npos) val = val.substr(s);
        else val = "";

        if (section == "listen") {
            if (key == "port") cfg->listen_port = std::stoi(val);
            else if (key == "threads") cfg->worker_threads = std::stoi(val);
        } else if (section == "health_check") {
            if (key == "interval") cfg->health_check.interval_s = std::stoi(val);
            else if (key == "timeout") cfg->health_check.timeout_ms = std::stoi(val);
        } else if (section == "limits") {
            if (key == "max_connections") cfg->limits.max_connections = std::stoi(val);
        } else if (key.size() >= 2 && key[0] == '-' && key[1] == ' ') {
            // upstream entry
            UpstreamConfig up;
            std::string entry = (key.size() > 2) ? key.substr(2) + val : val;
            size_t p = entry.find("port:");
            if (p != std::string::npos) up.port = std::stoi(entry.substr(p + 5));
            size_t h = entry.find("host:");
            if (h != std::string::npos) {
                size_t end = entry.find_first_of(" \t", h + 5);
                up.host = entry.substr(h + 5, end - h - 5);
            }
            up.weight = 1;
            cfg->upstreams.push_back(up);
        }
    }
    return cfg;
}

}  // namespace config
}  // namespace skynet
