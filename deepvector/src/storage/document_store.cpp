#include "dv/storage/document_store.h"
#include "minikv/options.h"
#include "minikv/slice.h"
#include "minikv/write_batch.h"
#include "minikv/status.h"
#include "core/db_impl.h"
#include <cstring>
#include <sstream>
#include <sys/stat.h>

namespace dv {
namespace storage {

// Simple binary encoding for DocumentMeta
static std::string encodeMeta(const DocumentMeta& meta) {
    std::ostringstream os;
    uint32_t text_len = static_cast<uint32_t>(meta.text.size());
    uint32_t tags_len = static_cast<uint32_t>(meta.tags.size());
    os.write(reinterpret_cast<const char*>(&text_len), 4);
    os.write(meta.text.data(), text_len);
    os.write(reinterpret_cast<const char*>(&tags_len), 4);
    os.write(meta.tags.data(), tags_len);
    os.write(reinterpret_cast<const char*>(&meta.timestamp), 8);
    return os.str();
}

static bool decodeMeta(const std::string& data, DocumentMeta& meta) {
    const char* p = data.data();
    const char* end = data.data() + data.size();
    if (end - p < 4) return false;
    uint32_t text_len;
    std::memcpy(&text_len, p, 4); p += 4;
    if (end - p < static_cast<std::ptrdiff_t>(text_len)) return false;
    meta.text.assign(p, text_len); p += text_len;
    if (end - p < 4) return false;
    uint32_t tags_len;
    std::memcpy(&tags_len, p, 4); p += 4;
    if (end - p < static_cast<std::ptrdiff_t>(tags_len)) return false;
    meta.tags.assign(p, tags_len); p += tags_len;
    if (end - p < 8) return false;
    std::memcpy(&meta.timestamp, p, 8);
    return true;
}

static std::string keyFor(uint64_t id) {
    return std::string(reinterpret_cast<const char*>(&id), 8);
}

class DocumentStore::Impl {
public:
    explicit Impl(const std::string& data_dir)
        : data_dir_(data_dir), count_(0) {
        ::mkdir(data_dir.c_str(), 0755);
        ::minikv::Options opts;
        opts.db_path = data_dir + "/meta";
        opts.wal_sync = true;
        auto status = ::minikv::core::DBImpl::open(opts, &db_);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open document store: " + status.ToString());
        }
    }

    ~Impl() { if (db_) db_.reset(); }

    void put(uint64_t id, const DocumentMeta& meta) {
        if (!db_) return;
        auto data = encodeMeta(meta);
        ::minikv::WriteOptions wopts{true};
        db_->put(wopts, ::minikv::Slice(keyFor(id)), ::minikv::Slice(data));
        count_++;
    }

    std::optional<DocumentMeta> get(uint64_t id) const {
        if (!db_) return std::nullopt;
        std::string value;
        ::minikv::ReadOptions ropts;
        auto s = db_->get(ropts, ::minikv::Slice(keyFor(id)), &value);
        if (!s.ok()) return std::nullopt;
        DocumentMeta meta;
        if (!decodeMeta(value, meta)) return std::nullopt;
        return meta;
    }

    void remove(uint64_t id) {
        if (!db_) return;
        ::minikv::WriteOptions wopts{true};
        db_->del(wopts, ::minikv::Slice(keyFor(id)));
        if (count_ > 0) count_--;
    }

    size_t count() const { return count_; }
    void flush() {}

private:
    std::string data_dir_;
    std::unique_ptr<::minikv::DB> db_;
    mutable size_t count_;
};

DocumentStore::DocumentStore(const std::string& data_dir)
    : impl_(std::make_unique<Impl>(data_dir)) {}

DocumentStore::~DocumentStore() = default;

void DocumentStore::put(uint64_t id, const DocumentMeta& meta) { impl_->put(id, meta); }
std::optional<DocumentMeta> DocumentStore::get(uint64_t id) const { return impl_->get(id); }
void DocumentStore::remove(uint64_t id) { impl_->remove(id); }
size_t DocumentStore::count() const { return impl_->count(); }
void DocumentStore::flush() { impl_->flush(); }

} // namespace storage
} // namespace dv
