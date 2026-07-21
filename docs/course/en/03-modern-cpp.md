# Module 03 — Modern C++ & Concurrency

> Source: [skip_list.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h), [thread_pool.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h), [lru_cache.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/lru_cache.h), [db_impl.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/db_impl.h)

## Background & Motivation

If you've ever chased a heap corruption bug at 2am, you already know why raw pointers are the interview minefield every senior engineer is tested on. Memory leaks, use-after-free, double-free, dangling references — C++ gives you enough rope to hang yourself, and distributed storage amplifies every mistake because the hot path runs millions of times per second. This module shows how modern C++ (C++11/17/20) hands you a safety net without sacrificing the zero-overhead principle: smart pointers, move semantics, lambdas, and a concurrency toolkit that lets the SkipList serve concurrent reads under a `shared_mutex`.

This is Module 03, building directly on the core syntax of Module 02 and setting up the concurrency vocabulary we'll reuse in Module 05's SkipList and beyond. We move from RAII basics to the full ownership story — `unique_ptr`, `shared_ptr`, `weak_ptr` — then cover move semantics, lambdas, `constexpr`, and the concurrency primitives (`mutex`, `shared_mutex`, `atomic`, `condition_variable`) that thread through minikv's thread pool and MemTable. By the end, the `shared_lock` in SkipList's `get` and the `unique_lock` in `put` will read like plain English.

After this module, you'll be able to answer "When does `shared_ptr`'s control block get allocated?", "Why mark move constructors `noexcept`?", "Why is `volatile` not a threading primitive?", and "How does `condition_variable::wait` defend against spurious wakeups?" You'll also be ready to implement a thread-safe data structure without reaching for a single raw `new` or `delete`.

## 1. Core Knowledge

- Smart pointers: `unique_ptr` (exclusive), `shared_ptr` (shared), `weak_ptr` (weak ref); control block & circular references.
- Move semantics: rvalue ref `T&&`, `std::move`, `std::forward`, move ctor/assign, the importance of `noexcept`.
- Lambdas: capture modes, `mutable`, `std::function` vs templates.
- `constexpr` / `constinit` / `consteval`: compile-time evaluation.
- Concurrency primitives: `std::thread`, `mutex`, `lock_guard` / `unique_lock` / `scoped_lock`, `shared_mutex` (RW lock), `atomic`, `condition_variable`.
- Memory orders: `relaxed` / `acquire` / `release` / `acq_rel` / `seq_cst`.
- Spurious wakeups and predicate waits.

## 2. Deep Dive

### 2.1 Smart Pointers

[db_impl.h:39-42](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/db_impl.h) manages WAL/Manifest/MemTable with `std::unique_ptr`:

```cpp
std::unique_ptr<WAL>      wal_;
std::unique_ptr<Manifest> manifest_;
std::unique_ptr<MemTable> memtable_;
```

Key points:

- `unique_ptr`: exclusive ownership, non-copyable, movable, zero-overhead (size = raw pointer, unless a custom deleter is used). `DBImpl` releases members automatically on destruction.
- `shared_ptr`: reference counted; the control block holds `use_count` (strong) + `weak_count` (weak); counts are atomic.
- `weak_ptr`: does not increase `use_count`; breaks circular references. `weak_ptr::lock()` upgrades to `shared_ptr` (returns empty if the object is gone).
- **Thread-safety granularity**: ref counts are atomically safe; but concurrent read/write of the *same* `shared_ptr` object is **not** safe — lock it.

```mermaid
graph TD
    U[unique_ptr&lt;T&gt;<br/>exclusive owner] -->|releases on destruction| T1[T object]
    S[shared_ptr&lt;T&gt;<br/>ref-counted owner] -->|increments use_count| T1
    W[weak_ptr&lt;T&gt;<br/>non-owning observer] -->|lock upgrades to| S
    S -.->|expires when use_count = 0| CB[Control Block<br/>use_count + weak_count]
    W -->|observes| CB
```

### 2.2 Move Semantics

[thread_pool.h:22-26](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) moves the task on submit:

```cpp
void submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));   // move into queue, avoid copying std::function
    }
    cv_.notify_one();
}
```

- `std::move(task)` is an unconditional cast to rvalue (`static_cast<T&&>`); it moves nothing itself. The actual move happens in `push`'s rvalue overload.
- Copying `std::function` may allocate (especially for lambdas capturing large objects); moving just swaps internal pointers — O(1).
- In `workerLoop`, `task = std::move(tasks_.front())` similarly moves on dequeue.

**Why `noexcept` matters**: when a `vector` grows, if the element's move ctor is not `noexcept`, the standard library falls back to copy for the strong exception guarantee. So prefer marking move ctors `noexcept`.

### 2.3 Lambdas

[thread_pool.h:17](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) starts a worker with a lambda:

```cpp
workers_.emplace_back([this] { workerLoop(); });
```

- `[this]` captures the `this` pointer, so the lambda can access `workerLoop`, `tasks_`, etc.
- Capture modes: `[]` (none), `[=]` (by value), `[&]` (by ref), `[this]`, `[x, &y]` (mixed).
- `cv_.wait(lock, [this] { return !tasks_.empty() || !running_; })` is a predicate wait, internally equivalent to `while(!pred) wait()` — automatically handles spurious wakeups.

### 2.4 `constexpr` Compile-Time Evaluation

A `constexpr` function/variable can be evaluated at compile time, with the result inlined — zero runtime cost. In minikv, [skip_list.h:25](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h) uses `static constexpr int kMaxLevel = 32;` for a compile-time constant.

C++20 adds `consteval` (forces compile-time evaluation, no runtime calls allowed) and `constinit` (forces compile-time initialization, preventing static-init-order issues).

### 2.5 The `shared_mutex` RW Lock

[skip_list.h:39-40](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h) takes a write lock in `put`:

```cpp
void put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);   // write lock (exclusive)
    // ... modify the skiplist ...
}
```

while `get` takes a read lock (see [skip_list.h:68-69](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h)):

```cpp
std::optional<std::string> get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);   // read lock (shared)
    // ... read-only access ...
}
```

- `shared_mutex`: read locks (`shared_lock`) can be held by many threads concurrently; write locks (`unique_lock`) are exclusive.
- Good for "read-heavy" workloads. MemTable has far more reads than writes before flush, so an RW lock fits.
- Caveat: some implementations favor readers and may starve writers; for write-heavy workloads use a plain `mutex` instead.

### 2.6 `atomic` and Memory Orders

[thread_pool.h:57](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) uses `std::atomic<bool> running_`:

```cpp
std::atomic<bool> running_;   // stop() sets false; workers poll it
```

- `atomic` guarantees indivisible operations and provides memory-order control.
- Memory orders (weak to strong):
  - `relaxed`: atomic only, no synchronization (good for counters).
  - `acquire` (load): subsequent reads/writes cannot be reordered before this load.
  - `release` (store): prior reads/writes cannot be reordered after this store.
  - `acq_rel`: read-modify-write, both acquire and release.
  - `seq_cst` (default): globally sequentially consistent, strongest but slowest.
- `volatile` **cannot** be used for thread synchronization — it only prevents compiler optimization, with no atomicity or memory-order guarantees.

### 2.7 `condition_variable` and Spurious Wakeups

[thread_pool.h:43-48](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) is a classic producer-consumer:

```cpp
std::unique_lock<std::mutex> lock(mutex_);
cv_.wait(lock, [this] { return !tasks_.empty() || !running_; });  // predicate wait
if (!running_ && tasks_.empty()) break;
task = std::move(tasks_.front());
tasks_.pop();
```

- **Must use `unique_lock`**, not `lock_guard`: `wait` internally needs to `unlock` (so producers can enqueue) + `relock`; `lock_guard` cannot unlock manually.
- **Predicate wait** `cv_.wait(lock, pred)` equals `while(!pred) cv_.wait(lock)`, auto-looping to defend against spurious wakeups.
- **Spurious wakeup**: the OS may wake a waiting thread for no reason even if the condition is unmet; without a predicate you must `while` manually.
- `notify_one` wakes one, `notify_all` wakes all.

### 2.8 `mutable` and const Members

[lru_cache.h:56](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/lru_cache.h) declares `mutex_` as `mutable`:

```cpp
private:
    size_t capacity_;
    mutable std::mutex mutex_;   // const member functions can still lock
```

`get` is a `const` member but needs to lock `mutex_` (the lock itself is mutable state). `mutable` allows modifying this member inside a const function. This is the classic "logical const vs physical const" distinction — locking a cache does not change its logically observable state.

## 3. Thinking Questions

1. Why is `unique_ptr` "zero-overhead"? How does its size differ from a raw pointer? When is there extra overhead?
2. After `std::move`, what state is the source object in? Can you still use it?
3. SkipList uses `shared_mutex` RW locks. If changed to a plain `mutex`, in what scenario would it actually be faster?
4. Is `cv_.wait(lock, pred)` equivalent to `while(!pred) cv_.wait(lock)`? Why prefer the former?
5. `atomic<bool>` defaults to `seq_cst`. Is `relaxed` enough for `ThreadPool::stop()` setting `running_` to false? Why?

## 4. Hands-on Exercises

### Exercise 4.1 (Hand-write a RW-locked SkipList, LeetCode 1206 advanced)

Following [skip_list.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h), implement a thread-safe skiplist: `put` takes a write lock, `get`/`entries` take read locks. Verify no races with 10 threads each inserting 10,000 keys.

### Exercise 4.2 (Extend the ThreadPool, per [thread_pool.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h))

Build on the existing `ThreadPool` and add:

1. `submit` returns `std::future<T>` (hint: use `std::packaged_task`).
2. Bounded queue (max N tasks); when full, apply a "caller-runs" policy.
3. Graceful shutdown: `stop()` still drains remaining queued tasks.

### Exercise 4.3 (Lock-free SPSC Queue)

Implement a single-producer single-consumer lock-free ring queue using `atomic` + `acquire/release`. Require `alignas(64)` to avoid false sharing. Write a test verifying 1M push/pop with no data corruption.

## 5. Self-Check

1. `unique_ptr` has ____ (exclusive/shared) ownership; `shared_ptr` has ____ ownership.
2. `std::move` is essentially ____________, and moves no data itself.
3. For `shared_mutex`, the read lock uses ____ and the write lock uses ____.
4. `condition_variable::wait` must pair with ____ (lock_guard/unique_lock) because wait needs ____.
5. `volatile` ____ (can/cannot) be used for thread synchronization, because it guarantees neither ____ nor ____.

<details>
<summary>Reference Answers</summary>

1. exclusive; shared
2. an unconditional cast to rvalue (static_cast<T&&>)
3. shared_lock; unique_lock
4. unique_lock; unlock+relock
5. cannot; atomicity; memory ordering

Thinking question key points:
1. Size usually equals a raw pointer (no custom deleter); with a custom deleter, unique_ptr must store it (may take 16 bytes). Zero-overhead means no runtime cost like ref counting.
2. "Valid but unspecified" state; resources are typically gone (e.g. pointer nulled). Safe to destruct or reassign, but do not read its value.
3. In write-heavy or read-write-balanced workloads, a plain mutex lets only one thread in, avoiding the RW lock's upgrade overhead and writer starvation — often faster.
4. Equivalent. The former is preferred because it's concise and you can't forget the loop.
5. Yes. `running_` is only a one-shot stop flag; no other data depends on it for happens-before (task sync relies on mutex/cv), so relaxed suffices.

</details>

---

← [Module 02](./02-cpp-core.md)  |  Next: [Module 04 — Go & TypeScript Basics](./04-go-ts.md) →
