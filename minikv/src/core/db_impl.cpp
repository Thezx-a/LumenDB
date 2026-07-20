#include "core/db_impl.h"
#include "core/sstable_builder.h"
#include "utils/coding.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

namespace minikv {
namespace core {

DBImpl::DBImpl(const Options& options) : options_(options), seq_(0) {
    db_path_ = options_.db_path;
    if (db_path_.empty()) db_path_ = "./minikv_data";
    ::mkdir(db_path_.c_str(), 0755);
    for (int i = 0; i <= options_.max_level; ++i) {
        std::string levelDir = db_path_ + "/level-" + std::to_string(i);
        ::mkdir(levelDir.c_str(), 0755);
    }
}

DBImpl::~DBImpl() {
    if (wal_) wal_->sync();
    if (compaction_mgr_) compaction_mgr_->stop();
}

Status DBImpl::open(const Options& options, std::unique_ptr<DB>* dbptr) {
    auto impl = std::make_unique<DBImpl>(options);
    Status s = impl->recover();
    if (!s.ok()) return s;
    impl->compaction_mgr_ = std::make_unique<CompactionManager>(&impl->version_, impl->db_path_);
    impl->compaction_mgr_->start();
    *dbptr = std::move(impl);
    return Status::Ok();
}

Status DBImpl::recover() {
    std::string wal_path = db_path_ + "/wal.log";
    wal_ = std::make_unique<WAL>(wal_path);
    memtable_ = std::make_unique<MemTable>(options_.memtable_size);
    auto records = wal_->replay();
    for (const auto& record : records) {
        const char* p = record.data();
        const char* end = record.data() + record.size();
        while (p < end) {
            uint8_t type = static_cast<uint8_t>(*p++);
            uint32_t keyLen = utils::decodeFixed32(p); p += 4;
            uint32_t valLen = utils::decodeFixed32(p); p += 4;
            Slice key(p, keyLen); p += keyLen;
            Slice value(p, valLen); p += valLen;
            bool isDelete = (type == 2);
            memtable_->put(key, value, seq_, isDelete);
            seq_++;
        }
    }
    return Status::Ok();
}

Status DBImpl::put(const WriteOptions& opts, const Slice& key, const Slice& value) {
    WriteBatch batch;
    batch.put(key, value);
    return write(opts, batch);
}

Status DBImpl::del(const WriteOptions& opts, const Slice& key) {
    WriteBatch batch;
    batch.del(key);
    return write(opts, batch);
}

Status DBImpl::write(const WriteOptions& opts, const WriteBatch& batch) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    uint64_t currentSeq = seq_.fetch_add(batch.count());
    for (const auto& op : batch.ops()) {
        uint64_t opSeq = ++currentSeq;
        bool isDel = (op.type == BatchOpType::kDelete);
        memtable_->put(Slice(op.key), Slice(op.value), opSeq, isDel);
    }
    if (wal_) {
        std::string data;
        for (const auto& op : batch.ops()) {
            data.push_back(static_cast<char>(static_cast<uint8_t>(op.type)));
            char lenBuf[4];
            utils::encodeFixed32(lenBuf, static_cast<uint32_t>(op.key.size()));
            data.append(lenBuf, 4);
            utils::encodeFixed32(lenBuf, static_cast<uint32_t>(op.value.size()));
            data.append(lenBuf, 4);
            data.append(op.key);
            data.append(op.value);
        }
        wal_->append(Slice(data));
        if (options_.wal_sync && opts.sync) {
            Status s = wal_->sync();
            if (!s.ok()) return s;
        }
    }
    maybeFlush();
    return Status::Ok();
}

Status DBImpl::get(const ReadOptions& opts, const Slice& key, std::string* value) {
    auto result = memtable_->get(key, seq_.load());
    if (result) {
        *value = std::move(*result);
        return Status::Ok();
    }
    if (immutable_memtable_) {
        result = immutable_memtable_->get(key, seq_.load());
        if (result) {
            *value = std::move(*result);
            return Status::Ok();
        }
    }
    // Check L0+ SSTables
    for (int level = 0; level <= options_.max_level; ++level) {
        auto files = version_.getLevelFiles(level);
        for (const auto& path : files) {
            auto reader = SSTableReader::open(path);
            if (!reader) continue;
            auto r = reader->get(key);
            if (r) {
                *value = std::move(*r);
                return Status::Ok();
            }
        }
    }
    return Status::NotFound();
}

void DBImpl::maybeFlush() {
    if (memtable_->shouldFlush()) {
        immutable_memtable_ = std::move(memtable_);
        memtable_ = std::make_unique<MemTable>(options_.memtable_size);
        flushMemTable();
    }
}

Status DBImpl::flushMemTable() {
    if (!immutable_memtable_ || immutable_memtable_->empty()) {
        immutable_memtable_.reset();
        return Status::Ok();
    }
    auto entries = immutable_memtable_->entries();
    std::string filePath = db_path_ + "/level-0/" +
        std::to_string(version_.nextFileNumber()) + ".sst";
    SSTableBuilder builder(filePath, options_.block_size);
    for (const auto& entry : entries) {
        char keyBuf[8];
        uint64_t ik = entry.internal_key;
        for (int i = 0; i < 8; ++i) keyBuf[i] = static_cast<char>((ik >> (i * 8)) & 0xFF);
        builder.add(entry.internal_key, Slice(keyBuf, 8), Slice(entry.value));
    }
    builder.finish();
    version_.addLevelFile(0, filePath);
    immutable_memtable_.reset();
    if (wal_) wal_->truncate();
    return Status::Ok();
}

std::unique_ptr<Iterator> DBImpl::newIterator(const ReadOptions& opts) {
    return nullptr;  // TODO: implement
}

void DBImpl::compact() {
    if (compaction_mgr_) compaction_mgr_->triggerCompaction();
}

}  // namespace core
}  // namespace minikv
