#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "dv/collection.h"
#include "dv/types.h"

namespace dv {
namespace server {

/**
 * Named collection registry — one Collection per logical namespace.
 * Disk layout: <root_dir>/<collection_name>/
 *
 * Thread-safe create/get/list/erase. Used by DeepVectorServer HTTP layer.
 */
class CollectionRegistry {
public:
    CollectionRegistry(std::string root_dir, CollectionConfig default_config);

    Collection* getOrCreate(const std::string& name);
    Collection* get(const std::string& name);
    bool erase(const std::string& name);
    std::vector<std::string> list() const;
    size_t size() const;

    const CollectionConfig& defaultConfig() const { return default_config_; }
    const std::string& rootDir() const { return root_dir_; }

private:
    std::string root_dir_;
    CollectionConfig default_config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<Collection>> collections_;
};

} // namespace server
} // namespace dv
