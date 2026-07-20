#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Submit a task and get a future
    template <typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    void WaitAll();
    size_t QueueSize() const;
    void Shutdown();

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::condition_variable finished_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_tasks_{0};
};

// Template implementation
template <typename F, typename... Args>
auto ThreadPool::Submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<return_type> result = task->get_future();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) throw std::runtime_error("Submit on stopped ThreadPool");
        tasks_.emplace([task]() { (*task)(); });
        active_tasks_++;
    }
    condition_.notify_one();
    return result;
}
