#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include "core/compaction.h"
#include "core/memtable.h"
#include "core/version.h"
#include "core/wal.h"
#include "minikv/db.h"
#include "minikv/options.h"

namespace minikv {
namespace core {

class DBImpl : public ::minikv::DB {
public:
    explicit DBImpl(const Options& options);
    ~DBImpl();

    static Status open(const Options& options, std::unique_ptr<DB>* dbptr);

    Status put(const WriteOptions& opts, const Slice& key, const Slice& value) override;
    Status get(const ReadOptions& opts, const Slice& key, std::string* value) override;
    Status del(const WriteOptions& opts, const Slice& key) override;
    Status write(const WriteOptions& opts, const WriteBatch& batch) override;
    std::unique_ptr<Iterator> newIterator(const ReadOptions& opts) override;
    void compact() override;

private:
    void maybeFlush();
    Status flushMemTable();
    Status recover();

    Options options_;
    std::string db_path_;
    std::unique_ptr<WAL> wal_;
    std::unique_ptr<MemTable> memtable_;
    std::unique_ptr<MemTable> immutable_memtable_;
    std::atomic<uint64_t> seq_;
    std::mutex write_mutex_;
    Version version_;
    std::unique_ptr<CompactionManager> compaction_mgr_;
};

}  // namespace core
}  // namespace minikv
