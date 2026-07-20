#include "skiplist.h"
#include "bloom.h"
#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <chrono>
#include <random>

void demo_skiplist() {
    std::cout << "=== SkipList MemTable Demo ===" << std::endl;

    SkipList<std::string, std::string> memtable;

    // Insert key-value pairs
    for (int i = 0; i < 100000; i++) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        memtable.Insert(key, value);
    }
    std::cout << "Inserted 100K keys" << std::endl;
    std::cout << "SkipList size: " << memtable.Size() << std::endl;

    // Point lookups
    auto val = memtable.Get("key_42");
    assert(val.has_value() && *val == "value_42");
    std::cout << "Point lookup OK: key_42 -> " << *val << std::endl;

    val = memtable.Get("key_99999");
    assert(val.has_value() && *val == "value_99999");
    std::cout << "Point lookup OK: key_99999 -> " << *val << std::endl;

    val = memtable.Get("nonexistent");
    assert(!val.has_value());
    std::cout << "Missing key returns nullopt OK" << std::endl;

    // Range scan
    std::vector<std::pair<std::string, std::string>> range;
    memtable.Scan("key_90000", "key_90010",
                  [&range](const std::string& k, const std::string& v) {
                      range.emplace_back(k, v);
                  });
    assert(range.size() == 11);
    std::cout << "Range scan [key_90000, key_90010]: " << range.size() << " entries" << std::endl;
    std::cout << "  First: " << range.front().first << " -> " << range.front().second << std::endl;
    std::cout << "  Last:  " << range.back().first << " -> " << range.back().second << std::endl;

    // Performance benchmark
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 99999);
    const int OPS = 1000000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < OPS; i++) {
        memtable.Get("key_" + std::to_string(dist(rng)));
    }
    auto end = std::chrono::steady_clock::now();
    double us = std::chrono::duration<double, std::micro>(end - start).count();
    std::cout << "Point lookup throughput: " << static_cast<int>(OPS / (us / 1000.0)) << " ops/sec" << std::endl;
}

void demo_bloom() {
    std::cout << "\n=== Bloom Filter Demo ===" << std::endl;

    const size_t N = 100000;
    const double FPR = 0.01;
    BloomFilter bloom(N, FPR, 0);

    std::cout << "Bloom filter: " << bloom.BitCount() << " bits, "
              << bloom.HashCount() << " hash functions" << std::endl;
    std::cout << "Expected FPR: ~" << FPR * 100 << "%" << std::endl;

    // Insert N keys
    for (size_t i = 0; i < N; i++) {
        bloom.Add("key_" + std::to_string(i));
    }

    // Check inserted keys (no false negatives)
    size_t true_positives = 0;
    for (size_t i = 0; i < N; i++) {
        if (bloom.MayContain("key_" + std::to_string(i))) true_positives++;
    }
    std::cout << "True positives: " << true_positives << "/" << N << std::endl;

    // Check absent keys (measure false positive rate)
    size_t false_positives = 0;
    const size_t TEST = 100000;
    for (size_t i = 0; i < TEST; i++) {
        if (bloom.MayContain("absent_" + std::to_string(i))) false_positives++;
    }
    double measured_fpr = static_cast<double>(false_positives) / TEST;
    std::cout << "False positives: " << false_positives << "/" << TEST
              << " = " << measured_fpr * 100 << "%" << std::endl;
}

void demo_integrated() {
    std::cout << "\n=== Integrated: SkipList + Bloom Filter ===" << std::endl;

    SkipList<std::string, std::string> memtable;
    BloomFilter bloom(10000, 0.01, 0);

    // Simulate LSM-Tree write path
    for (int i = 0; i < 10000; i++) {
        std::string key = "user:" + std::to_string(i);
        std::string value = "data_" + std::to_string(i * 42);
        memtable.Insert(key, value);
        bloom.Add(key);
    }

    // Simulate read path with bloom filter check
    std::string query = "user:5000";
    if (bloom.MayContain(query)) {
        // Bloom says "maybe" - check memtable
        auto val = memtable.Get(query);
        if (val) {
            std::cout << "Found: " << query << " -> " << *val << std::endl;
        } else {
            std::cout << "Bloom false positive (key not in memtable)" << std::endl;
        }
    } else {
        std::cout << "Bloom says definitely not present - skip SSTable" << std::endl;
    }

    // Non-existent key
    query = "user:99999";
    if (!bloom.MayContain(query)) {
        std::cout << "Bloom correctly filtered out non-existent key: " << query << std::endl;
    }
}

int main() {
    demo_skiplist();
    demo_bloom();
    demo_integrated();
    std::cout << "\nAll demos completed successfully!" << std::endl;
    return 0;
}
