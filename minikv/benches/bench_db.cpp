#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include "core/db_impl.h"
#include "minikv/options.h"
using namespace minikv;

int main() {
    Options opts;
    opts.db_path = "/tmp/minikv_bench";
    opts.wal_sync = false;
    ::system("rm -rf /tmp/minikv_bench");

    std::unique_ptr<DB> db;
    auto s = core::DBImpl::open(opts, &db);
    if (!s.ok()) { std::cerr << s.ToString() << std::endl; return 1; }

    int N = 1000000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string val = "val_" + std::to_string(i);
        db->put(WriteOptions{false}, Slice(key), Slice(val));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto put_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "PUT " << N << " ops in " << put_ms << "ms => " << (N * 1000.0 / put_ms) << " ops/s" << std::endl;

    t0 = std::chrono::high_resolution_clock::now();
    std::string val;
    for (int i = 0; i < N; ++i) {
        std::string key = "key_" + std::to_string(i);
        db->get(ReadOptions{}, Slice(key), &val);
    }
    t1 = std::chrono::high_resolution_clock::now();
    auto get_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "GET " << N << " ops in " << get_ms << "ms => " << (N * 1000.0 / get_ms) << " ops/s" << std::endl;

    ::system("rm -rf /tmp/minikv_bench");
    return 0;
}
