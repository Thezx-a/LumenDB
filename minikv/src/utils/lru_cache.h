#pragma once
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace minikv {
namespace utils {

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.erase(it->second);
        }
        list_.push_front({key, value});
        map_[key] = list_.begin();
        if (map_.size() > capacity_) {
            auto last = list_.end();
            --last;
            map_.erase(last->first);
            list_.pop_back();
        }
    }

    void erase(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.erase(it->second);
            map_.erase(it);
        }
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

private:
    size_t capacity_;
    mutable std::mutex mutex_;
    using ListType = std::list<std::pair<K, V>>;
    ListType list_;
    std::unordered_map<K, typename ListType::iterator> map_;
};

}  // namespace utils
}  // namespace minikv
