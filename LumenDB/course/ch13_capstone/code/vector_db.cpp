#include "vector_db.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

static const char MAGIC[8] = {'L','U','M','E','N','V','0','1'};

VectorStorage::VectorStorage(const VectorDBConfig& config) : config_(config) {
    fd_ = open(config_.data_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ < 0) { perror("open"); exit(1); }

    struct stat st;
    if (fstat(fd_, &st) < 0) { perror("fstat"); exit(1); }

    if (st.st_size > 0) {
        mmap_size_ = st.st_size;
    } else {
        mmap_size_ = header_size_ + config_.initial_capacity
                   * config_.dimension * sizeof(float);
        if (ftruncate(fd_, mmap_size_) < 0) { perror("ftruncate"); exit(1); }
    }

    mmap_base_ = mmap(nullptr, mmap_size_, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd_, 0);
    if (mmap_base_ == MAP_FAILED) { perror("mmap"); exit(1); }

    char* base = static_cast<char*>(mmap_base_);
    if (st.st_size == 0) {
        memcpy(base, MAGIC, 8);
        *reinterpret_cast<int32_t*>(base + 8) = config_.dimension;
        *reinterpret_cast<int64_t*>(base + 12) = 0;
        *reinterpret_cast<int64_t*>(base + 20) = config_.dimension * sizeof(float);
    } else {
        if (memcmp(base, MAGIC, 8) != 0) {
            fprintf(stderr, "Invalid file format\n"); exit(1);
        }
    }

    count_ptr_ = reinterpret_cast<int64_t*>(base + 12);
    data_base_ = reinterpret_cast<float*>(base + header_size_);
}

VectorStorage::~VectorStorage() {
    sync();
    if (mmap_base_ && mmap_base_ != MAP_FAILED)
        munmap(mmap_base_, mmap_size_);
    if (fd_ >= 0) close(fd_);
}

int64_t VectorStorage::append(const float* vector) {
    resize_if_needed();
    int64_t id = *count_ptr_;
    memcpy(data_base_ + id * config_.dimension, vector,
           config_.dimension * sizeof(float));
    (*count_ptr_)++;
    return id + 1;
}

const float* VectorStorage::get_vector(int64_t id) const {
    if (id < 1 || id > *count_ptr_) return nullptr;
    return data_base_ + (id - 1) * config_.dimension;
}

int64_t VectorStorage::count() const { return *count_ptr_; }
size_t VectorStorage::dimension() const { return config_.dimension; }
void VectorStorage::sync() {
    if (mmap_base_ && mmap_base_ != MAP_FAILED)
        msync(mmap_base_, mmap_size_, MS_SYNC);
}

void VectorStorage::resize_if_needed() {
    int64_t current = *count_ptr_;
    int64_t capacity = (mmap_size_ - header_size_)
                     / (config_.dimension * sizeof(float));
    if (current >= capacity) {
        size_t new_size = mmap_size_ * 2;
        munmap(mmap_base_, mmap_size_);
        if (ftruncate(fd_, new_size) < 0) { perror("ftruncate"); exit(1); }
        mmap_base_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd_, 0);
        if (mmap_base_ == MAP_FAILED) { perror("mmap"); exit(1); }
        mmap_size_ = new_size;
        count_ptr_ = reinterpret_cast<int64_t*>(
            static_cast<char*>(mmap_base_) + 12);
        data_base_ = reinterpret_cast<float*>(
            static_cast<char*>(mmap_base_) + header_size_);
    }
}

HNSWIndex::HNSWIndex(const HNSWConfig& config, VectorStorage* storage)
    : cfg_(config), storage_(storage) {}

float HNSWIndex::distance(const float* a, const float* b) const {
    float sum = 0.0f;
    for (size_t i = 0; i < cfg_.dimension; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

int HNSWIndex::random_level() {
    return (int)(-std::log(uni_(rng_)) * cfg_.mL);
}

void HNSWIndex::insert(int64_t id, const float* vector) {
    int level = random_level();
    HNSWNode node{id, level};
    node.neighbors.resize(level + 1);

    if (nodes_.empty()) {
        entry_point_ = id;
        max_level_ = level;
        nodes_[id] = node;
        return;
    }

    int64_t cur_ep = entry_point_;
    for (int l = max_level_; l > level; l--) {
        std::vector<std::pair<int64_t, float>> cand;
        search_layer(vector, cur_ep, l, 1, cand);
        cur_ep = cand[0].first;
    }

    for (int l = std::min(level, max_level_); l >= 0; l--) {
        std::vector<std::pair<int64_t, float>> cand;
        search_layer(vector, cur_ep, l, cfg_.ef_construction, cand);
        size_t Mmax = (l == 0) ? cfg_.M_max0 : cfg_.M;
        select_neighbors(vector, cand, Mmax, node.neighbors[l]);

        for (int64_t nid : node.neighbors[l]) {
            auto& nb = nodes_[nid];
            if (nb.neighbors[l].size() < Mmax) {
                nb.neighbors[l].push_back(id);
            } else {
                float max_d = -1;
                size_t max_i = 0;
                for (size_t j = 0; j < nb.neighbors[l].size(); j++) {
                    float d = distance(
                        storage_->get_vector(nid),
                        storage_->get_vector(nb.neighbors[l][j]));
                    if (d > max_d) { max_d = d; max_i = j; }
                }
                float d_new = distance(
                    storage_->get_vector(nid), vector);
                if (d_new < max_d) nb.neighbors[l][max_i] = id;
            }
        }
        cur_ep = cand[0].first;
    }

    if (level > max_level_) { max_level_ = level; entry_point_ = id; }
    nodes_[id] = node;
}

void HNSWIndex::search_layer(
    const float* query, int64_t entry, int level, int ef,
    std::vector<std::pair<int64_t, float>>& out)
{
    std::unordered_set<int64_t> visited{entry};
    float ed = distance(query, storage_->get_vector(entry));

    std::priority_queue<std::pair<float, int64_t>,
        std::vector<std::pair<float, int64_t>>,
        std::greater<>> cand_q;
    cand_q.push({ed, entry});

    std::priority_queue<std::pair<float, int64_t>> result;
    result.push({ed, entry});

    while (!cand_q.empty()) {
        auto [d, cur] = cand_q.top(); cand_q.pop();
        float worst = result.top().first;
        if (d > worst && (int)result.size() >= ef) break;

        for (int64_t nb : nodes_[cur].neighbors[level]) {
            if (visited.count(nb)) continue;
            visited.insert(nb);
            float nd = distance(query, storage_->get_vector(nb));
            worst = result.top().first;
            if (nd < worst || (int)result.size() < ef) {
                cand_q.push({nd, nb});
                result.push({nd, nb});
                if ((int)result.size() > ef) result.pop();
            }
        }
    }

    out.clear();
    while (!result.empty()) {
        out.emplace_back(result.top()); result.pop();
    }
    std::reverse(out.begin(), out.end());
}

void HNSWIndex::select_neighbors(const float* query,
    const std::vector<std::pair<int64_t, float>>& in,
    size_t M_max, std::vector<int64_t>& out)
{
    out.clear();
    size_t count = std::min(in.size(), M_max);
    out.reserve(count);
    for (size_t i = 0; i < count; i++) {
        out.push_back(in[i].first);
    }
}

std::vector<SearchResult> HNSWIndex::search(const float* query, int k, int ef) {
    if (ef == 0) ef = k * 2;
    if (nodes_.empty()) return {};

    int64_t cur_ep = entry_point_;
    for (int l = max_level_; l > 0; l--) {
        std::vector<std::pair<int64_t, float>> cand;
        search_layer(query, cur_ep, l, 1, cand);
        cur_ep = cand[0].first;
    }

    std::vector<std::pair<int64_t, float>> cand;
    search_layer(query, cur_ep, 0, ef, cand);

    std::vector<SearchResult> results;
    size_t count = std::min(cand.size(), (size_t)k);
    results.reserve(count);
    for (size_t i = 0; i < count; i++) {
        results.push_back({cand[i].first, cand[i].second});
    }
    return results;
}

std::vector<SearchResult> brute_force_search(
    const VectorStorage& storage, const float* query, int k)
{
    std::priority_queue<std::pair<float, int64_t>,
        std::vector<std::pair<float, int64_t>>,
        std::greater<std::pair<float, int64_t>>> heap;

    int64_t n = storage.count();
    size_t dim = storage.dimension();
    for (int64_t i = 1; i <= n; i++) {
        float sum = 0.0f;
        const float* v = storage.get_vector(i);
        for (size_t d = 0; d < dim; d++) {
            float diff = query[d] - v[d];
            sum += diff * diff;
        }
        float dist = std::sqrt(sum);
        heap.push({dist, i});
        if ((int)heap.size() > k) heap.pop();
    }

    std::vector<SearchResult> results;
    while (!heap.empty()) {
        results.push_back({heap.top().second, heap.top().first});
        heap.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}
