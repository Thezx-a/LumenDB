#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "minikv/slice.h"
#include "utils/hash.h"

namespace minikv {
namespace core {

class BloomFilter {
public:
    BloomFilter(size_t expected_keys, double false_positive_rate = 0.01)
        : bits_per_key_(0), num_hashes_(0) {
        int bits = static_cast<int>(-1.0 * std::log(false_positive_rate) / std::log(2.0) / std::log(2.0));
        num_hashes_ = static_cast<int>(bits * std::log(2.0));
        if (num_hashes_ < 1) num_hashes_ = 1;
        if (num_hashes_ > 30) num_hashes_ = 30;
        bits_per_key_ = static_cast<int>(static_cast<double>(bits) / std::log(2.0));
        if (bits_per_key_ < 1) bits_per_key_ = 1;
        bits_.assign(expected_keys * bits_per_key_ / 8 + 1, 0);
    }

    void add(const Slice& key) {
        uint32_t h1 = utils::murmurHash2(key.data(), key.size(), 0xbc9f1d34);
        uint32_t h2 = utils::murmurHash2(key.data(), key.size(), 0x9e3779b9);
        uint32_t bits = static_cast<uint32_t>(bits_.size() * 8);
        for (int i = 0; i < num_hashes_; ++i) {
            uint32_t pos = (h1 + i * h2) % bits;
            bits_[pos / 8] |= (1 << (pos % 8));
        }
    }

    bool mightContain(const Slice& key) const {
        uint32_t h1 = utils::murmurHash2(key.data(), key.size(), 0xbc9f1d34);
        uint32_t h2 = utils::murmurHash2(key.data(), key.size(), 0x9e3779b9);
        uint32_t bits = static_cast<uint32_t>(bits_.size() * 8);
        for (int i = 0; i < num_hashes_; ++i) {
            uint32_t pos = (h1 + i * h2) % bits;
            if ((bits_[pos / 8] & (1 << (pos % 8))) == 0) return false;
        }
        return true;
    }

void persist(const std::string& path) const {
        std::ofstream ofs(path, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(&num_hashes_), sizeof(num_hashes_));
        ofs.write(reinterpret_cast<const char*>(&bits_per_key_), sizeof(bits_per_key_));
        uint64_t sz = bits_.size();
        ofs.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
        ofs.write(reinterpret_cast<const char*>(bits_.data()), bits_.size());
    }

    static std::unique_ptr<BloomFilter> load(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return nullptr;
        int nh, bk;
        uint64_t sz;
        ifs.read(reinterpret_cast<char*>(&nh), sizeof(nh));
        ifs.read(reinterpret_cast<char*>(&bk), sizeof(bk));
        ifs.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        auto bf = std::unique_ptr<BloomFilter>(new BloomFilter(1, 0.01));
        bf->num_hashes_ = nh;
        bf->bits_per_key_ = bk;
        bf->bits_.resize(sz);
        ifs.read(reinterpret_cast<char*>(bf->bits_.data()), sz);
        return bf;
    }

    size_t memoryUsage() const { return bits_.size(); }
    int numHashes() const { return num_hashes_; }

private:
    int bits_per_key_;
    int num_hashes_;
    std::vector<uint8_t> bits_;
};

}  // namespace core
}  // namespace minikv
