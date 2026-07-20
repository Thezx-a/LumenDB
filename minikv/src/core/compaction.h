#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "core/sstable_reader.h"
#include "core/version.h"
#include "minikv/status.h"

namespace minikv {
namespace core {

class CompactionManager {
public:
    CompactionManager(Version* version, const std::string& db_path,
                      size_t block_size = 4096);
    ~CompactionManager();

    void start();
    void stop();
    void triggerCompaction();

private:
    void compactionLoop();
    Status compactL0();
    Status compactLevel(int level);

    Version* version_;
    std::string db_path_;
    size_t block_size_;
    std::thread compact_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> triggered_;
};

}  // namespace core
}  // namespace minikv
