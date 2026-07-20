#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace minikv {
namespace core {

struct SSTableMeta {
    std::string path;
    std::string min_key;
    std::string max_key;
    uint64_t file_number;
    uint64_t file_size;
};

class Version {
public:
    Version();

    std::vector<std::string> getLevelFiles(int level) const;
    std::vector<SSTableMeta> getLevelMetas(int level) const;
    void addLevelFile(int level, const std::string& path);
    void removeLevelFiles(int level, const std::vector<std::string>& paths);
    bool shouldCompactL0() const;
    size_t levelSize(int level) const;
    uint64_t nextFileNumber();

private:
    mutable std::mutex mutex_;
    std::vector<std::vector<SSTableMeta>> levels_;
    std::atomic<uint64_t> next_file_number_;
};

}  // namespace core
}  // namespace minikv
