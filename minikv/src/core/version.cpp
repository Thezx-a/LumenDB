#include "core/version.h"

#include <algorithm>

#include "core/manifest.h"

namespace minikv {
namespace core {

Version::Version() : next_file_number_(1) {
    levels_.resize(7);
}

Version::~Version() = default;

void Version::restoreFrom(const std::vector<std::vector<SSTableMeta>>& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    levels_ = snapshot;
    if (levels_.size() < 7) levels_.resize(7);
    // Bump next_file_number past the largest observed file number.
    uint64_t max_no = 0;
    for (const auto& lvl : levels_) {
        for (const auto& meta : lvl) {
            if (meta.file_number > max_no) max_no = meta.file_number;
        }
    }
    uint64_t cur = next_file_number_.load();
    if (max_no >= cur) next_file_number_.store(max_no + 1);
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
    if (level < 0) return;
    if (level >= static_cast<int>(levels_.size())) levels_.resize(level + 1);
    uint64_t file_no = next_file_number_++;
    levels_[level].push_back({path, "", "", file_no, 0});
    // Persist through Manifest if connected.
    if (manifest_) {
        (void)manifest_->recordAddFile(level, path, file_no);
        (void)manifest_->sync();
    }
}

void Version::removeLevelFiles(int level, const std::vector<std::string>& paths) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < 0 || level >= static_cast<int>(levels_.size())) return;
    auto& files = levels_[level];
    files.erase(std::remove_if(files.begin(), files.end(),
        [&](const SSTableMeta& m) {
            bool removed = std::find(paths.begin(), paths.end(), m.path) != paths.end();
            if (removed && manifest_) {
                (void)manifest_->recordRemoveFile(level, m.path, m.file_number);
            }
            return removed;
        }), files.end());
    if (manifest_) (void)manifest_->sync();
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
    return next_file_number_.fetch_add(1);
}

}  // namespace core
}  // namespace minikv