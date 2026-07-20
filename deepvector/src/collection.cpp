#include "dv/collection.h"
#include "dv/index/hnsw.h"
#include "dv/index/distance.h"
#include "dv/storage/vector_store.h"
#include "dv/storage/document_store.h"
#include "dv/quantize/pq.h"
#include "dv/quantize/scalar.h"
#include <functional>
#include <cmath>
#include <algorithm>
#include <fstream>

namespace dv {

static constexpr size_t kPQTrainThreshold = 256;
static constexpr size_t kSQTrainThreshold = 100;

Collection::Collection(const CollectionConfig& config, const std::string& data_dir)
    : config_(config), data_dir_(data_dir) {
    store_ = std::make_unique<storage::VectorStore>(config.dim, data_dir + "/vectors.bin");
    docs_ = std::make_unique<storage::DocumentStore>(data_dir + "/docs");

    index_ = std::make_unique<index::HNSWIndex>(config.hnsw_m, config.hnsw_ef_construction);
    index_->setEfSearch(config.hnsw_ef_search);

    if (config_.use_pq) {
        size_t M = config_.pq_M ? config_.pq_M : config_.dim / 4;
        pq_ = std::make_unique<quantize::ProductQuantizer>(config_.dim, M, config_.pq_K);
        pq_scratch_a_.resize(pq_->M());
        pq_scratch_b_.resize(pq_->M());
        pq_dist_table_.resize(pq_->M() * pq_->K());
    }
    if (config_.use_sq) {
        sq_ = std::make_unique<quantize::ScalarQuantizer>(config_.dim);
        sq_scratch_a_.resize(config_.dim);
        sq_scratch_b_.resize(config_.dim);
    }

    index_->setPairwiseDistance([this](uint64_t a, uint64_t b) -> float {
        const float* va = store_->get(a);
        const float* vb = store_->get(b);
        if (!va || !vb) return std::numeric_limits<float>::max();

        if (pq_ && pq_->isTrained()) {
            pq_->encode(va, pq_scratch_a_.data());
            pq_->encode(vb, pq_scratch_b_.data());
            return pq_->symmetricDistance(pq_scratch_a_.data(), pq_scratch_b_.data());
        }
        if (sq_ && sq_->isTrained()) {
            sq_->encode(va, sq_scratch_a_.data());
            sq_->encode(vb, sq_scratch_b_.data());
            return sq_->l2SquaredDistance(sq_scratch_a_.data(), sq_scratch_b_.data());
        }
        return index::compute_distance(va, vb, config_.dim, config_.metric);
    });

    index_->setQueryDistance([this](uint64_t id, const float* query) -> float {
        const float* v = store_->get(id);
        if (!v) return std::numeric_limits<float>::max();

        if (pq_ && pq_->isTrained()) {
            pq_->computeDistanceTable(query, pq_dist_table_.data());
            pq_->encode(v, pq_scratch_a_.data());
            return pq_->asymmetricDistance(pq_scratch_a_.data(), pq_dist_table_.data());
        }
        if (sq_ && sq_->isTrained()) {
            sq_->encode(query, sq_scratch_a_.data());
            sq_->encode(v, sq_scratch_b_.data());
            return sq_->l2SquaredDistance(sq_scratch_a_.data(), sq_scratch_b_.data());
        }
        return index::compute_distance(v, query, config_.dim, config_.metric);
    });
}

Collection::~Collection() {
    if (store_) store_->flush();
}

void Collection::trainQuantizers() {
    size_t n = train_data_.size() / config_.dim;
    if (n == 0) return;

    const float* vecs = train_data_.data();

    if (pq_ && !pq_->isTrained() && n >= kPQTrainThreshold) {
        pq_->train(vecs, n);
    }
    if (sq_ && !sq_->isTrained() && n >= kSQTrainThreshold) {
        sq_->train(vecs, n);
    }
}

uint64_t Collection::add(const float* vector) {
    if (config_.use_pq || config_.use_sq) {
        train_data_.insert(train_data_.end(), vector, vector + config_.dim);
        trainQuantizers();
    }

    uint64_t id = store_->append(vector);
    index_->insert(id);
    return id;
}

uint64_t Collection::add(const float* vector, const storage::DocumentMeta& meta) {
    if (config_.use_pq || config_.use_sq) {
        train_data_.insert(train_data_.end(), vector, vector + config_.dim);
        trainQuantizers();
    }

    uint64_t id = store_->append(vector);
    index_->insert(id);
    docs_->put(id, meta);
    return id;
}

void Collection::remove(uint64_t id) {
    store_->remove(id);
    index_->remove(id);
    docs_->remove(id);
}

std::vector<SearchResult> Collection::search(const float* query, size_t k) const {
    return index_->search(query, k);
}

std::vector<SearchResult> Collection::searchWithFilter(const float* query, size_t k,
                                                        const FilterNode& filter) const {
    std::vector<SearchResult> results;
    size_t ef = index_->getEfSearch();
    size_t max_attempts = 10;

    for (size_t attempt = 0; attempt < max_attempts && results.size() < k; ++attempt) {
        size_t prev_ef = ef;
        ef = std::max(ef, (attempt + 1) * 20);
        const_cast<index::HNSWIndex*>(index_.get())->setEfSearch(ef);

        auto candidates = index_->search(query, std::max(k * 10, ef));

        const_cast<index::HNSWIndex*>(index_.get())->setEfSearch(prev_ef);

        auto fieldAccessor = [this](uint64_t id, const std::string& field) -> std::string {
            auto meta = docs_->get(id);
            if (!meta) return "";
            if (field == "tags") return meta->tags;
            if (field == "text") return meta->text;
            if (field == "timestamp") return std::to_string(meta->timestamp);
            return "";
        };

        for (auto& cand : candidates) {
            if (evaluateFilter(filter, cand.id, fieldAccessor)) {
                results.push_back(cand);
                if (results.size() >= k) break;
            }
        }

        if (candidates.size() < ef) break;
    }

    return results;
}

const float* Collection::getVector(uint64_t id) const {
    return store_->get(id);
}

std::optional<storage::DocumentMeta> Collection::getMeta(uint64_t id) const {
    return docs_->get(id);
}

size_t Collection::size() const {
    return index_->size();
}

Dimension Collection::dim() const {
    return config_.dim;
}

void Collection::save(const std::string& name) {
    store_->flush();
    docs_->flush();

    // Save config to a JSON file
    std::ofstream cfg_file(data_dir_ + "/" + name + ".cfg.json");
    if (cfg_file.is_open()) {
        cfg_file << "{";
        cfg_file << "\"dim\":" << config_.dim << ",";
        cfg_file << "\"metric\":" << static_cast<int>(config_.metric) << ",";
        cfg_file << "\"hnsw_m\":" << config_.hnsw_m << ",";
        cfg_file << "\"hnsw_ef_construction\":" << config_.hnsw_ef_construction << ",";
        cfg_file << "\"hnsw_ef_search\":" << config_.hnsw_ef_search << ",";
        cfg_file << "\"use_pq\":" << (config_.use_pq ? "true" : "false") << ",";
        cfg_file << "\"pq_M\":" << config_.pq_M << ",";
        cfg_file << "\"pq_K\":" << config_.pq_K << ",";
        cfg_file << "\"use_sq\":" << (config_.use_sq ? "true" : "false") << "";
        cfg_file << "}";
    }
}

std::unique_ptr<Collection> Collection::load(const std::string& name, const std::string& data_dir) {
    // Load config from JSON file
    CollectionConfig config;
    config.dim = 768;  // default fallback
    config.metric = DistanceMetric::Cosine;

    std::ifstream cfg_file(data_dir + "/" + name + ".cfg.json");
    if (cfg_file.is_open()) {
        std::string line, content;
        while (std::getline(cfg_file, line)) {
            content += line;
        }

        auto find_value = [&content](const std::string& key) -> std::string {
            auto pos = content.find("\"" + key + "\":");
            if (pos == std::string::npos) return "";
            pos = content.find(':', pos) + 1;
            while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
            size_t end = content.find_first_of(",}", pos);
            if (end == std::string::npos) end = content.size();
            return content.substr(pos, end - pos);
        };

        auto dim_str = find_value("dim");
        if (!dim_str.empty()) config.dim = static_cast<Dimension>(std::stoul(dim_str));

        auto metric_str = find_value("metric");
        if (!metric_str.empty()) config.metric = static_cast<DistanceMetric>(std::stoul(metric_str));

        auto m_str = find_value("hnsw_m");
        if (!m_str.empty()) config.hnsw_m = std::stoul(m_str);

        auto ef_str = find_value("hnsw_ef_construction");
        if (!ef_str.empty()) config.hnsw_ef_construction = std::stoul(ef_str);

        auto ef_s_str = find_value("hnsw_ef_search");
        if (!ef_s_str.empty()) config.hnsw_ef_search = std::stoul(ef_s_str);

        config.use_pq = find_value("use_pq") == "true";
        auto pq_m_str = find_value("pq_M");
        if (!pq_m_str.empty()) config.pq_M = std::stoul(pq_m_str);
        auto pq_k_str = find_value("pq_K");
        if (!pq_k_str.empty()) config.pq_K = std::stoul(pq_k_str);
        config.use_sq = find_value("use_sq") == "true";
    }

    auto coll = std::make_unique<Collection>(config, data_dir + "/" + name);

    // Rebuild index from all stored vectors
    size_t n = coll->size();
    if (n == 0) return coll;

    // For now, we just return the collection with an empty index
    // and vectors will be re-added on demand
    // The mmap store is loaded automatically by the Collection constructor
    return coll;
}

} // namespace dv
