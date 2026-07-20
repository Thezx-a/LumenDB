#include <chrono>
#include <iostream>
#include <vector>
#include "core/skip_list.h"
using namespace minikv::core;

int main() {
    for (int N : {1000, 10000, 100000, 1000000}) {
        SkipList sl;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i)
            sl.put(static_cast<uint64_t>(i), std::to_string(i));
        auto t1 = std::chrono::high_resolution_clock::now();
        auto put_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) sl.get(static_cast<uint64_t>(i));
        t1 = std::chrono::high_resolution_clock::now();
        auto get_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        std::cout << "N=" << N << " put=" << put_ms << "ms "
                  << "get=" << get_ms << "ms "
                  << "mem=" << sl.approximateMemoryUsage() / 1024 << "KB" << std::endl;
    }
    return 0;
}
