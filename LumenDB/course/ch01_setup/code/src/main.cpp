#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <random>
#include <algorithm>

enum class DistanceMetric { L2, IP, Cosine };

struct CollectionConfig {
    int dim = 3;
    DistanceMetric metric = DistanceMetric::L2;
};

struct SearchResult {
    int id;
    float distance;
};

class Collection {
public:
    Collection(CollectionConfig cfg, const std::string& /*path*/)
        : cfg_(cfg) {}

    void add(const float* vec) {
        int id = static_cast<int>(vectors_.size());
        vectors_.emplace_back(vec, vec + cfg_.dim);
        ids_.push_back(id);
    }

    std::vector<SearchResult> search(const float* query, int k) const {
        std::vector<std::pair<float, int>> distances;
        distances.reserve(vectors_.size());
        for (size_t i = 0; i < vectors_.size(); i++) {
            float d = compute_distance(query, vectors_[i].data());
            distances.emplace_back(d, ids_[i]);
        }
        std::partial_sort(distances.begin(),
                          distances.begin() + std::min(k, (int)distances.size()),
                          distances.end());
        std::vector<SearchResult> results;
        int limit = std::min(k, (int)distances.size());
        for (int i = 0; i < limit; i++) {
            results.push_back({distances[i].second, distances[i].first});
        }
        return results;
    }

private:
    float compute_distance(const float* a, const float* b) const {
        switch (cfg_.metric) {
            case DistanceMetric::L2: {
                float sum = 0.0f;
                for (int i = 0; i < cfg_.dim; i++) {
                    float diff = a[i] - b[i];
                    sum += diff * diff;
                }
                return std::sqrt(sum);
            }
            case DistanceMetric::IP: {
                float sum = 0.0f;
                for (int i = 0; i < cfg_.dim; i++)
                    sum += a[i] * b[i];
                return -sum;
            }
            case DistanceMetric::Cosine: {
                float dot = 0.0f, na = 0.0f, nb = 0.0f;
                for (int i = 0; i < cfg_.dim; i++) {
                    dot += a[i] * b[i];
                    na  += a[i] * a[i];
                    nb  += b[i] * b[i];
                }
                float denom = std::sqrt(na) * std::sqrt(nb);
                if (denom < 1e-9f) return 1.0f;
                return 1.0f - dot / denom;
            }
        }
        return 0.0f;
    }

    CollectionConfig cfg_;
    std::vector<std::vector<float>> vectors_;
    std::vector<int> ids_;
};

int main() {
    CollectionConfig cfg;
    cfg.dim = 3;
    cfg.metric = DistanceMetric::L2;

    Collection coll(cfg, "./data");

    std::vector<float> v1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f, 0.0f};
    std::vector<float> q  = {1.0f, 0.1f, 0.0f};

    coll.add(v1.data());
    coll.add(v2.data());

    auto results = coll.search(q.data(), 2);
    for (auto& r : results) {
        std::cout << "id=" << r.id << " dist=" << r.distance << std::endl;
    }
    return 0;
}
