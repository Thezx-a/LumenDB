#include "pimpl.h"
#include "type_erasure.h"
#include "thread_pool.h"
#include <iostream>
#include <thread>
#include <cassert>
#include <vector>
#include <chrono>
#include <atomic>

void demo_pimpl() {
    std::cout << "=== PIMPL Pattern Demo ===" << std::endl;

    Widget w;
    w.Set("x", 10);
    w.Set("y", 20);
    w.Set("z", 30);

    auto val = w.Get("x");
    assert(val.has_value() && *val == 10);
    std::cout << "Widget Get(x): " << *val << std::endl;

    val = w.Get("y");
    assert(val.has_value() && *val == 20);
    std::cout << "Widget Get(y): " << *val << std::endl;

    val = w.Get("missing");
    assert(!val.has_value());
    std::cout << "Widget Get(missing): nullopt" << std::endl;

    std::cout << "Widget size: " << w.Size() << std::endl;

    // Move semantics
    Widget w2 = std::move(w);
    val = w2.Get("x");
    assert(val.has_value() && *val == 10);
    std::cout << "Move construct: w2.Get(x) = " << *val << std::endl;

    std::cout << "PIMPL demo passed!" << std::endl;
}

void demo_type_erasure() {
    std::cout << "\n=== Type Erasure Demo ===" << std::endl;

    // Free function
    DistanceIndex idx1;
    idx1.SetDistance(distance_functions::l2_distance);
    float a[] = {1.0f, 2.0f, 3.0f};
    float b[] = {4.0f, 5.0f, 6.0f};
    std::cout << "L2 distance: " << idx1.Compute(a, b, 3) << std::endl;

    // Lambda
    DistanceIndex idx2;
    idx2.SetDistance([](const float* x, const float* y, int dim) -> float {
        float sum = 0.0f;
        for (int i = 0; i < dim; ++i) sum += std::abs(x[i] - y[i]);
        return sum;
    });
    std::cout << "L1 distance: " << idx2.Compute(a, b, 3) << std::endl;

    // Functor
    DistanceIndex idx3;
    idx3.SetDistance(distance_functions::CosineDistance{});
    float c[] = {1.0f, 0.0f, 0.0f};
    float d[] = {0.0f, 1.0f, 0.0f};
    std::cout << "Cosine distance (orthogonal): " << idx3.Compute(c, d, 3) << std::endl;

    float e[] = {1.0f, 1.0f, 1.0f};
    float f[] = {1.0f, 1.0f, 1.0f};
    std::cout << "Cosine distance (identical): " << idx3.Compute(e, f, 3) << std::endl;

    // std::function with captures
    DistanceIndex idx4;
    float scale = 2.0f;
    idx4.SetDistance([scale](const float* x, const float* y, int dim) -> float {
        float sum = 0.0f;
        for (int i = 0; i < dim; ++i) {
            float d = (x[i] - y[i]) * scale;
            sum += d * d;
        }
        return std::sqrt(sum);
    });
    std::cout << "Scaled L2 (2x): " << idx4.Compute(a, b, 3) << std::endl;

    std::cout << "Type erasure demo passed!" << std::endl;
}

void demo_thread_pool() {
    std::cout << "\n=== Thread Pool Demo ===" << std::endl;

    ThreadPool pool(4);
    std::atomic<int> counter{0};

    // Submit multiple tasks
    const int NUM_TASKS = 100;
    std::vector<std::future<int>> futures;

    for (int i = 0; i < NUM_TASKS; ++i) {
        futures.push_back(pool.Submit([i, &counter]() -> int {
            counter++;
            // Simulate work
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            return i * i;
        }));
    }

    // Collect results
    long long sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }
    std::cout << "Submitted " << NUM_TASKS << " tasks" << std::endl;
    std::cout << "Counter value: " << counter.load() << std::endl;
    std::cout << "Sum of squares: " << sum << std::endl;

    // Verify sum of squares formula: n*(n-1)*(2n-1)/6
    long long expected = static_cast<long long>(NUM_TASKS) * (NUM_TASKS - 1) * (2 * NUM_TASKS - 1) / 6;
    assert(sum == expected);
    std::cout << "Sum verified: " << sum << " == " << expected << std::endl;

    pool.WaitAll();
    std::cout << "All tasks completed!" << std::endl;
}

void demo_concurrent_widget() {
    std::cout << "\n=== Concurrent Widget (shared_mutex) Demo ===" << std::endl;

    Widget w;
    const int NUM_WRITERS = 4;
    const int NUM_READERS = 8;
    const int OPS_PER_THREAD = 1000;
    std::atomic<int> read_ops{0};
    std::atomic<int> write_ops{0};

    // Register change callback
    w.OnChange([&write_ops](const std::string&, int, int) {
        write_ops++;
    });

    auto writer = [&](int id) {
        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            std::string key = "key_" + std::to_string(id) + "_" + std::to_string(i);
            w.Set(key, i);
        }
    };

    auto reader = [&](int id) {
        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            std::string key = "key_" + std::to_string(id % NUM_WRITERS) + "_" + std::to_string(i);
            w.Get(key);
            read_ops++;
        }
    };

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_WRITERS; ++i) threads.emplace_back(writer, i);
    for (int i = 0; i < NUM_READERS; ++i) threads.emplace_back(reader, i);
    for (auto& t : threads) t.join();

    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "Read ops: " << read_ops.load() << std::endl;
    std::cout << "Write ops: " << write_ops.load() << std::endl;
    std::cout << "Widget size: " << w.Size() << std::endl;
    std::cout << "Time: " << ms << " ms" << std::endl;
    std::cout << "Throughput: " << static_cast<int>((read_ops.load() + write_ops.load()) / (ms / 1000.0)) << " ops/sec" << std::endl;
}

int main() {
    demo_pimpl();
    demo_type_erasure();
    demo_thread_pool();
    demo_concurrent_widget();
    std::cout << "\nAll C++ pattern demos completed successfully!" << std::endl;
    return 0;
}
