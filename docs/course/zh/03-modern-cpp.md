# Module 03 — 现代 C++ 与并发

> 对应源码：[skip_list.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h)、[thread_pool.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h)、[lru_cache.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/lru_cache.h)、[db_impl.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/db_impl.h)

## 1. 核心知识

- 智能指针：`unique_ptr`（独占）、`shared_ptr`（共享）、`weak_ptr`（弱引用）；控制块与循环引用。
- 移动语义：右值引用 `T&&`、`std::move`、`std::forward`、移动构造/赋值、`noexcept` 的重要性。
- Lambda：捕获方式、`mutable`、`std::function` 与模板的区别。
- `constexpr` / `constinit` / `consteval`：编译期求值。
- 并发原语：`std::thread`、`mutex`、`lock_guard` / `unique_lock` / `scoped_lock`、`shared_mutex`（读写锁）、`atomic`、`condition_variable`。
- 内存序：`relaxed` / `acquire` / `release` / `acq_rel` / `seq_cst`。
- 虚假唤醒与谓词 wait。

## 2. 内容详解

### 2.1 智能指针

[db_impl.h:39-42](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/db_impl.h) 用 `std::unique_ptr` 管理 WAL/Manifest/MemTable：

```cpp
std::unique_ptr<WAL>      wal_;
std::unique_ptr<Manifest> manifest_;
std::unique_ptr<MemTable> memtable_;
```

要点：

- `unique_ptr`：独占所有权，不可拷贝、可移动，零开销（大小 = 裸指针，除非自定义删除器）。`DBImpl` 析构时成员自动释放。
- `shared_ptr`：引用计数，控制块含 `use_count`（强）+ `weak_count`（弱），计数为原子操作。
- `weak_ptr`：不增加 `use_count`，解决循环引用。`weak_ptr::lock()` 升级为 `shared_ptr`（若对象已释放返回空）。
- **线程安全粒度**：引用计数本身原子安全；但同一 `shared_ptr` 对象的并发读写**不**安全，需加锁。

### 2.2 移动语义

[thread_pool.h:22-26](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) 提交任务时移动：

```cpp
void submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));   // 移动入队，避免拷贝 std::function
    }
    cv_.notify_one();
}
```

- `std::move(task)` 是无条件的右值转换（`static_cast<T&&>`），本身不移动任何东西；真正的移动发生在 `push` 的右值重载里。
- `std::function` 拷贝可能涉及堆分配（尤其捕获大对象的 lambda），移动只需交换内部指针，O(1)。
- `workerLoop` 里 `task = std::move(tasks_.front())` 同理，出队也用移动。

**`noexcept` 的重要性**：`vector` 扩容时，若元素移动构造非 `noexcept`，标准库为强异常保证会回退到拷贝。因此移动构造应尽量标 `noexcept`。

### 2.3 Lambda

[thread_pool.h:17](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) 用 lambda 启动 worker：

```cpp
workers_.emplace_back([this] { workerLoop(); });
```

- `[this]` 捕获 `this` 指针，使 lambda 内可访问 `workerLoop`、`tasks_` 等成员。
- 捕获方式：`[]`（无）、`[=]`（按值）、`[&]`（按引用）、`[this]`、`[x, &y]`（混合）。
- `cv_.wait(lock, [this] { return !tasks_.empty() || !running_; })` 是带谓词的 wait，内部等价于 `while(!pred) wait()`，自动处理虚假唤醒。

### 2.4 `constexpr` 编译期求值

`constexpr` 标注的函数/变量可在编译期求值，结果内联到代码，零运行期开销。minikv 中 [skip_list.h:25](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h) 用 `static constexpr int kMaxLevel = 32;` 定义编译期常量。

C++20 新增 `consteval`（强制编译期求值，不允许运行期调用）与 `constinit`（强制编译期初始化，防止静态初始化顺序问题）。

### 2.5 读写锁 `shared_mutex`

[skip_list.h:39-40](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h) 的 `put` 用写锁：

```cpp
void put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);   // 写锁（独占）
    // ... 修改跳表 ...
}
```

而 `get` 用读锁（见 [skip_list.h:68-69](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h)）：

```cpp
std::optional<std::string> get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);   // 读锁（共享）
    // ... 只读访问 ...
}
```

- `shared_mutex`：读锁（`shared_lock`）可多线程并发持有；写锁（`unique_lock`）独占。
- 适用「读多写少」场景。MemTable 在 flush 前读远多于写，故用读写锁。
- 注意：部分实现偏向读者，可能写者饥饿；写多场景应改用普通 `mutex`。

### 2.6 `atomic` 与内存序

[thread_pool.h:57](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) 用 `std::atomic<bool> running_`：

```cpp
std::atomic<bool> running_;   // stop() 设 false，worker 轮询
```

- `atomic` 保证操作不可分割，且提供内存序控制。
- 内存序（从弱到强）：
  - `relaxed`：只保证原子，无同步语义（计数器适用）。
  - `acquire`（读）：之后的读写不能重排到该读之前。
  - `release`（写）：之前的读写不能重排到该写之后。
  - `acq_rel`：读改写操作，同时 acquire + release。
  - `seq_cst`（默认）：全局顺序一致，最强但最慢。
- `volatile` **不能**用于多线程同步——它只防编译器优化，不保证原子性也不保证内存序。

### 2.7 `condition_variable` 与虚假唤醒

[thread_pool.h:43-48](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h) 是经典生产者-消费者：

```cpp
std::unique_lock<std::mutex> lock(mutex_);
cv_.wait(lock, [this] { return !tasks_.empty() || !running_; });  // 谓词 wait
if (!running_ && tasks_.empty()) break;
task = std::move(tasks_.front());
tasks_.pop();
```

- **必须用 `unique_lock`** 而非 `lock_guard`：`wait` 内部需要 `unlock`（让生产者入队）+ `relock`，`lock_guard` 不支持手动解锁。
- **谓词 wait** `cv_.wait(lock, pred)` 等价于 `while(!pred) cv_.wait(lock)`，自动循环检查，防御虚假唤醒。
- **虚假唤醒**：OS 可能无端唤醒等待线程，即使条件未满足；若不用谓词而用裸 `wait`，必须手动 `while` 循环。
- `notify_one` 唤醒一个，`notify_all` 唤醒所有。

### 2.8 `mutable` 与 const 成员

[lru_cache.h:56](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/lru_cache.h) 的 `mutex_` 声明为 `mutable`：

```cpp
private:
    size_t capacity_;
    mutable std::mutex mutex_;   // const 成员函数也能加锁
```

`get` 是 `const` 成员函数，但要加锁修改 `mutex_`（锁本身是可变状态），`mutable` 允许在 const 函数内修改该成员。这是「逻辑 const vs 物理 const」的经典区分——缓存加锁不改变逻辑可见状态。

## 3. 思考题

1. `unique_ptr` 为什么是「零开销」？它和裸指针在大小上有何差异？何时会有额外开销？
2. `std::move` 之后原对象处于什么状态？还能用吗？
3. SkipList 用 `shared_mutex` 读写锁，如果改成普通 `mutex`，在什么场景下反而更快？
4. `cv_.wait(lock, pred)` 与 `while(!pred) cv_.wait(lock)` 等价吗？为什么推荐前者？
5. `atomic<bool>` 默认 `seq_cst`，`ThreadPool::stop()` 把 `running_` 设 false 用 `relaxed` 行不行？为什么？

## 4. 动手题

### 题 4.1（手撕读写锁版 SkipList，LeetCode 1206 进阶）

参考 [skip_list.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h)，实现一个线程安全跳表：`put` 用写锁，`get`/`entries` 用读锁。用 10 个线程各插入 10000 个 key 验证无竞争。

### 题 4.2（线程池扩展，对应 [thread_pool.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/thread_pool.h)）

在现有 `ThreadPool` 基础上添加：

1. `submit` 返回 `std::future<T>`（提示：用 `std::packaged_task`）。
2. 有界队列（最大 N 个任务），满时按「调用者执行」策略。
3. 优雅关闭：`stop()` 后仍执行完队列中剩余任务。

### 题 4.3（无锁 SPSC 队列）

实现一个单生产者单消费者无锁环形队列，用 `atomic` + `acquire/release`。要求 `alignas(64)` 避免 false sharing。写测试验证 100 万次 push/pop 无数据错乱。

## 5. 自检

1. `unique_ptr` 是____（独占/共享）所有权，`shared_ptr` 是____所有权。
2. `std::move` 本质是____________，不移动任何数据。
3. `shared_mutex` 的读锁用____，写锁用____。
4. `condition_variable::wait` 必须配合____（lock_guard/unique_lock），因为 wait 需要____。
5. `volatile` ____（能/不能）用于多线程同步，因为它不保证____和____。

<details>
<summary>参考答案</summary>

1. 独占；共享
2. 无条件的右值转换（static_cast<T&&>）
3. shared_lock；unique_lock
4. unique_lock；unlock+relock
5. 不能；原子性；内存序

思考题要点：
1. 大小通常等于裸指针（无自定义删除器时）；带自定义删除器时 unique_ptr 需额外存删除器（可能占 16 字节）。零开销指运行期无引用计数等额外成本。
2. 处于「有效但未指定」状态，通常资源已被搬走（如指针置空）；可安全析构或重新赋值，但不应读取其值。
3. 写多读少或读写相当的场景下，普通 mutex 一次只让一个线程进，避免读写锁的升级开销和写者饥饿，可能更快。
4. 等价。前者更推荐是因为代码简洁且不易遗漏循环。
5. 行。`running_` 只用作一次性停止标志，无其他数据依赖它建立 happens-before（任务同步靠 mutex/cv），用 relaxed 足够。

</details>

---

← [Module 02](./02-cpp-core.md)  |  下一模块：[Module 04 — Go 与 TypeScript 基础](./04-go-ts.md) →
