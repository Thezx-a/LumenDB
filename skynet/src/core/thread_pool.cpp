#include "skynet/core/thread_pool.h"
namespace skynet {
namespace core {

ThreadPool::ThreadPool(size_t num_threads) : running_(true) {
    for (size_t i = 0; i < num_threads; ++i)
        workers_.emplace_back([this] { workerLoop(); });
}

ThreadPool::~ThreadPool() { stop(); }

void ThreadPool::submit(std::function<void()> task) {
    { std::lock_guard<std::mutex> lock(mutex_); tasks_.push(std::move(task)); }
    cv_.notify_one();
}

void ThreadPool::stop() {
    running_ = false;
    cv_.notify_all();
    for (auto& t : workers_) if (t.joinable()) t.join();
}

void ThreadPool::workerLoop() {
    while (running_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !tasks_.empty() || !running_; });
            if (!running_ && tasks_.empty()) break;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

}  // namespace core
}  // namespace skynet
