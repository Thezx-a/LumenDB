#include "dv/server/collection_registry.h"
#include <sys/stat.h>
#include <iostream>

namespace dv {
namespace server {

CollectionRegistry::CollectionRegistry(std::string root_dir, CollectionConfig default_config)
    : root_dir_(std::move(root_dir)), default_config_(std::move(default_config)) {
    ::mkdir(root_dir_.c_str(), 0755);
}

Collection* CollectionRegistry::getOrCreate(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = collections_.find(name);
    if (it != collections_.end()) return it->second.get();

    std::string path = root_dir_ + "/" + name;
    ::mkdir(path.c_str(), 0755);
    auto coll = std::make_unique<Collection>(default_config_, path);
    Collection* ptr = coll.get();
    collections_[name] = std::move(coll);
    std::cout << "DeepVector: collection ready name=" << name
              << " dim=" << ptr->dim() << " path=" << path << std::endl;
    return ptr;
}

Collection* CollectionRegistry::get(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = collections_.find(name);
    return it == collections_.end() ? nullptr : it->second.get();
}

bool CollectionRegistry::erase(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return collections_.erase(name) > 0;
}

std::vector<std::string> CollectionRegistry::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(collections_.size());
    for (const auto& kv : collections_) names.push_back(kv.first);
    return names;
}

size_t CollectionRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return collections_.size();
}

} // namespace server
} // namespace dv
