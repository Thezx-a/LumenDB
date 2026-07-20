#include "core/version.h"
#include <algorithm>

namespace minikv {
namespace core {

Version::Version() : next_file_number_(1) {
    levels_.resize(7);
}

std::vector<std::string> Version::getLevelFiles(int level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    if (level < 0 || level >= static_cast<int>(levels_.size())) return result;
    for (const auto& meta : levels_[level]) result.push_back(meta.path);
    return result;
}

std::vector<SSTableMeta> Version::getLevelMetas(int level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < 0 || level >= static_cast<int>(levels_.size())) return {};
    return levels_[level];
}

void Version::addLevelFile(int level, const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < 0 || level >= static_cast<int>(levels_.size())) return;
    levels_[level].push_back({path, "", "", next_file_number_++, 0});
}

void Version::removeLevelFiles(int level, const std::vector<std::string>& paths) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < 0 || level >= static_cast<int>(levels_.size())) return;
    auto& files = levels_[level];
    files.erase(std::remove_if(files.begin(), files.end(),
        [&](const SSTableMeta& m) {
            return std::find(paths.begin(), paths.end(), m.path) != paths.end();
        }), files.end());
}

bool Version::shouldCompactL0() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !levels_.empty() && levels_[0].size() >= 4;
}

size_t Version::levelSize(int level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < 0 || level >= static_cast<int>(levels_.size())) return 0;
    return levels_[level].size();
}

uint64_t Version::nextFileNumber() {
    return next_file_number_++;
}

}  // namespace core
}  // namespace minikv
