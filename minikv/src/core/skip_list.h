#pragma once
#include <atomic>
#include <mutex>
#include <optional>
#include <random>
#include <shared_mutex>
#include <vector>
#include "minikv/slice.h"

namespace minikv {
namespace core {

inline uint64_t packInternalKey(const Slice& userKey, uint64_t seq, uint8_t type) {
    uint64_t h = 0;
    for (size_t i = 0; i < userKey.size(); ++i) h = h * 31 + static_cast<unsigned char>(userKey[i]);
    return (h << 16) | ((seq & 0xFFF) << 4) | (type & 0xF);
}

struct SkipNode {
    uint64_t key;
    std::string value;
    std::vector<SkipNode*> forward;
    SkipNode(uint64_t k, std::string v, int level)
        : key(k), value(std::move(v)), forward(level + 1, nullptr) {}
};

class SkipList {
public:
    static constexpr int kMaxLevel = 32;

    SkipList() : head_(new SkipNode(0, "", kMaxLevel)), max_level_(0), mem_usage_(0) {}

    ~SkipList() {
        SkipNode* node = head_;
        while (node) {
            SkipNode* next = node->forward[0];
            delete node;
            node = next;
        }
    }

    void put(uint64_t key, const std::string& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        std::vector<SkipNode*> update(kMaxLevel + 1, nullptr);
        SkipNode* x = head_;
        for (int i = max_level_; i >= 0; --i) {
            while (x->forward[i] && x->forward[i]->key < key) x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[0];
        if (x && x->key == key) {
            mem_usage_ -= x->value.size();
            x->value = value;
            mem_usage_ += value.size();
        } else {
            int level = randomLevel();
            if (level > max_level_) {
                for (int i = max_level_ + 1; i <= level; ++i) update[i] = head_;
                max_level_ = level;
            }
            x = new SkipNode(key, value, level);
            for (int i = 0; i <= level; ++i) {
                x->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = x;
            }
            mem_usage_ += sizeof(SkipNode) + value.size() + sizeof(key);
        }
    }

    std::optional<std::string> get(uint64_t key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        SkipNode* x = head_;
        for (int i = max_level_; i >= 0; --i) {
            while (x->forward[i] && x->forward[i]->key < key) x = x->forward[i];
        }
        x = x->forward[0];
        if (x && x->key == key) return x->value;
        return std::nullopt;
    }

    void del(uint64_t key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        std::vector<SkipNode*> update(kMaxLevel + 1, nullptr);
        SkipNode* x = head_;
        for (int i = max_level_; i >= 0; --i) {
            while (x->forward[i] && x->forward[i]->key < key) x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[0];
        if (x && x->key == key) {
            for (int i = 0; i <= max_level_; ++i) {
                if (update[i]->forward[i] != x) break;
                update[i]->forward[i] = x->forward[i];
            }
            mem_usage_ -= sizeof(SkipNode) + x->value.size() + sizeof(x->key);
            delete x;
            while (max_level_ > 0 && head_->forward[max_level_] == nullptr) --max_level_;
        }
    }

    std::vector<std::pair<uint64_t, std::string>> entries() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<std::pair<uint64_t, std::string>> result;
        SkipNode* x = head_->forward[0];
        while (x) {
            result.push_back({x->key, x->value});
            x = x->forward[0];
        }
        return result;
    }

    size_t approximateMemoryUsage() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return mem_usage_;
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return head_->forward[0] == nullptr;
    }

private:
    int randomLevel() {
        static thread_local std::mt19937 rng(std::random_device{}());
        static thread_local std::uniform_int_distribution<int> dist(0, 1);
        int level = 0;
        while (dist(rng) && level < kMaxLevel) ++level;
        return level;
    }

    mutable std::shared_mutex mutex_;
    SkipNode* head_;
    int max_level_;
    size_t mem_usage_;
};

}  // namespace core
}  // namespace minikv
