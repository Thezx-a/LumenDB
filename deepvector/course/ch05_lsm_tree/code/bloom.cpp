#include "bloom.h"
#include <cmath>
#include <cstring>

BloomFilter::BloomFilter(size_t num_bits, int num_hashes)
    : bits_((num_bits + 7) / 8, 0), k_(num_hashes) {}

BloomFilter::BloomFilter(size_t expected_count, double false_positive_rate, int /*dummy*/) {
    double m = -static_cast<double>(expected_count) * std::log(false_positive_rate) / (std::log(2.0) * std::log(2.0));
    size_t num_bits = static_cast<size_t>(std::ceil(m));
    bits_.assign((num_bits + 7) / 8, 0);
    k_ = static_cast<int>(std::ceil(static_cast<double>(num_bits) / expected_count * std::log(2.0)));
    if (k_ < 1) k_ = 1;
}

uint32_t BloomFilter::Hash(int seed, const char* data, size_t len) const {
    uint32_t h = 0x811c9dc5u; // FNV offset basis
    for (size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint32_t>(static_cast<unsigned char>(data[i]));
        h *= 0x01000193u; // FNV prime
    }
    h ^= static_cast<uint32_t>(seed);
    h *= 0x01000193u;
    return h;
}

void BloomFilter::Add(const std::string& key) {
    Add(key.data(), key.size());
}

void BloomFilter::Add(const char* data, size_t len) {
    uint32_t h1 = Hash(0, data, len);
    uint32_t h2 = Hash(h1, data, len);
    size_t num_bits = bits_.size() * 8;

    for (int i = 0; i < k_; ++i) {
        uint32_t pos = h1 + i * h2;
        pos %= static_cast<uint32_t>(num_bits);
        bits_[pos / 8] |= (1u << (pos % 8));
    }
}

bool BloomFilter::MayContain(const std::string& key) const {
    return MayContain(key.data(), key.size());
}

bool BloomFilter::MayContain(const char* data, size_t len) const {
    uint32_t h1 = Hash(0, data, len);
    uint32_t h2 = Hash(h1, data, len);
    size_t num_bits = bits_.size() * 8;

    for (int i = 0; i < k_; ++i) {
        uint32_t pos = h1 + i * h2;
        pos %= static_cast<uint32_t>(num_bits);
        if (!(bits_[pos / 8] & (1u << (pos % 8)))) {
            return false;
        }
    }
    return true;
}
