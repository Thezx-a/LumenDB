#pragma once
#include <cstddef>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace skynet {
namespace core {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 2);
    ~ThreadPool();
    void submit(std::function<void()> task);
    void stop();

private:
    void workerLoop();
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
};

}  // namespace core
}  // namespace skynet
