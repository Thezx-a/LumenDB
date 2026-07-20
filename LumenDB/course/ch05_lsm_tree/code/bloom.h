#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <functional>

class BloomFilter {
public:
    // num_bits: size of the bit array in bits
    // num_hashes: number of hash functions
    BloomFilter(size_t num_bits, int num_hashes);

    // Convenience: auto-calculate optimal params for expected_count items
    // with target false_positive_rate (e.g., 0.01 for 1%)
    BloomFilter(size_t expected_count, double false_positive_rate, int dummy);

    void Add(const std::string& key);
    void Add(const char* data, size_t len);
    bool MayContain(const std::string& key) const;
    bool MayContain(const char* data, size_t len) const;

    size_t BitCount() const { return bits_.size(); }
    int HashCount() const { return k_; }

private:
    uint32_t Hash(int seed, const char* data, size_t len) const;

    std::vector<uint8_t> bits_;
    int k_; // number of hash functions
};
