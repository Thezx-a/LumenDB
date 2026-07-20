#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "lumendb/types.h"

namespace lumendb {
namespace storage {

struct DocumentMeta {
    std::string text;
    std::string tags;
    int64_t timestamp;
};

class DocumentStore {
public:
    explicit DocumentStore(const std::string& data_dir);
    ~DocumentStore();

    void put(uint64_t id, const DocumentMeta& meta);
    std::optional<DocumentMeta> get(uint64_t id) const;
    void remove(uint64_t id);
    size_t count() const;
    void flush();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace storage
} // namespace lumendb
