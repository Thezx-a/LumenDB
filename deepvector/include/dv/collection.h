#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "dv/types.h"
#include "dv/filter.h"
#include "dv/storage/document_store.h"

namespace dv {
namespace index { class HNSWIndex; }
namespace storage { class VectorStore; }
namespace quantize {
    class ProductQuantizer;
    class ScalarQuantizer;
}

class Collection {
public:
    explicit Collection(const CollectionConfig& config, const std::string& data_dir = ".");
    ~Collection();

    uint64_t add(const float* vector);
    uint64_t add(const float* vector, const storage::DocumentMeta& meta);

    void remove(uint64_t id);

    std::vector<SearchResult> search(const float* query, size_t k = 10) const;

    std::vector<SearchResult> searchWithFilter(const float* query, size_t k,
                                                const FilterNode& filter) const;

    const float* getVector(uint64_t id) const;

    std::optional<storage::DocumentMeta> getMeta(uint64_t id) const;

    size_t size() const;
    Dimension dim() const;

    void save(const std::string& name);
    static std::unique_ptr<Collection> load(const std::string& name, const std::string& data_dir = ".");

private:
    void trainQuantizers();

    CollectionConfig config_;
    std::string data_dir_;
    std::unique_ptr<index::HNSWIndex> index_;
    std::unique_ptr<storage::VectorStore> store_;
    std::unique_ptr<storage::DocumentStore> docs_;

    std::unique_ptr<quantize::ProductQuantizer> pq_;
    std::unique_ptr<quantize::ScalarQuantizer> sq_;

    std::vector<float> train_data_;

    mutable std::vector<uint8_t> pq_scratch_a_;
    mutable std::vector<uint8_t> pq_scratch_b_;
    mutable std::vector<int8_t> sq_scratch_a_;
    mutable std::vector<int8_t> sq_scratch_b_;
    mutable std::vector<float> pq_dist_table_;
};

} // namespace dv
