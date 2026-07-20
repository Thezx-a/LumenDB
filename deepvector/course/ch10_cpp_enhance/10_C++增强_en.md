# Chapter 10: C++ Core Enhancements

> New DeepVector server endpoints and persistence support.

## Prerequisites

> 📎 **Reference**: [Build Environment](../prerequisites/01_构建环境配置_en.md) | [Testing](../prerequisites/04_测试框架_en.md)

---

## Learning Objectives

- Master DeepVector C++ Server endpoint design
- Understand Collection::load() persistence
- Learn JSON request handling in C++

---

## 10.1 New Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/collections` | GET | List all collections |
| `/batch/search` | POST | Batch search (multiple queries) |
| `/vector?id=` | GET | Get vector by ID |

---

## 10.2 Batch Search Implementation

```cpp
if (path == "/batch/search" && method == "POST") {
    auto req = json::parse(body);
    json resp;
    resp["results"] = json::array();

    for (auto& q : req["queries"]) {
        std::vector<float> vec = q["vector"].get<std::vector<float>>();
        auto results = collection_->search(vec.data(), q.value("k", 10));

        json batch;
        batch["results"] = json::array();
        for (auto& r : results) {
            json item;
            item["id"] = r.id;
            item["distance"] = r.distance;
            batch["results"].push_back(item);
        }
        resp["results"].push_back(batch);
    }
    return buildResponse(200, "application/json", resp.dump());
}
```

---

## 10.3 Collection::load() Persistence

```cpp
std::unique_ptr<Collection> Collection::load(
    const std::string& name, const std::string& data_dir
) {
    // 1. Restore CollectionConfig from JSON config file
    // 2. Create Collection instance (auto-loads mmap data)
    // 3. Return instance (HNSW index needs rebuilding)

    CollectionConfig config;
    // parse data_dir + name + ".cfg.json"
    auto coll = std::make_unique<Collection>(config, data_dir + "/" + name);
    return coll;
}
```

> **Note**: Currently `load()` returns a Collection without HNSW index.
> Vector data (mmap) is auto-loaded, but search needs index rebuild.

---

## Review Questions

1. Why doesn't `Collection::load()` auto-rebuild the HNSW index? When should it?
2. How to make `/batch/search` truly parallel (multi-threaded)?
3. What's the issue with the current HTTP server's handling of >64KB JSON bodies?

## Hands-on Exercises

1. Add parallel C++ threads to `/batch/search`
2. Implement HNSW index rebuilding in `Collection::load()`
3. Add a `/save` endpoint for API-triggered persistence
