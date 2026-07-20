#include "core/compaction.h"
#include "core/sstable_builder.h"
#include <unistd.h>
#include <chrono>
#include <iostream>

namespace minikv {
namespace core {

CompactionManager::CompactionManager(Version* version, const std::string& db_path,
                                     size_t block_size)
    : version_(version), db_path_(db_path), block_size_(block_size),
      running_(false), triggered_(false) {}

CompactionManager::~CompactionManager() { stop(); }

void CompactionManager::start() {
    running_ = true;
    compact_thread_ = std::thread([this] { compactionLoop(); });
}

void CompactionManager::stop() {
    running_ = false;
    if (compact_thread_.joinable()) compact_thread_.join();
}

void CompactionManager::triggerCompaction() { triggered_ = true; }

void CompactionManager::compactionLoop() {
    while (running_) {
        if (triggered_ || version_->shouldCompactL0()) {
            triggered_ = false;
            Status s = compactL0();
            if (!s.ok()) std::cerr << "Compaction failed: " << s.message() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

Status CompactionManager::compactL0() {
    auto l0_files = version_->getLevelFiles(0);
    if (l0_files.empty()) return Status::Ok();

    // Simplified: merge all L0 files into a single new L1 file
    std::vector<std::pair<uint64_t, std::pair<std::string, std::string>>> merged;

    for (const auto& path : l0_files) {
        auto reader = SSTableReader::open(path);
        if (!reader) continue;
        // Would iterate reader entries here and merge
    }

    // Create new SSTable at L1
    std::string newFile = db_path_ + "/level-1/" + std::to_string(
        version_->nextFileNumber()) + ".sst";
    SSTableBuilder builder(newFile, block_size_);
    // Would add merged entries here
    builder.finish();

    // Update version: remove old L0 files, add new L1 file
    version_->removeLevelFiles(0, l0_files);
    version_->addLevelFile(1, newFile);

    // Delete old L0 SSTable files
    for (const auto& path : l0_files) ::unlink(path.c_str());

    return Status::Ok();
}

Status CompactionManager::compactLevel(int level) {
    // L_n -> L_n+1 compaction
    // Similar to compactL0 but for L1+
    return Status::Ok();
}

}  // namespace core
}  // namespace minikv
