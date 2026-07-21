# Module 02 — C++ 核心语法

> 对应源码：[slice.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/slice.h)、[status.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/status.h)、[coding.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/coding.h)、[db.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/db.h)

## 背景与动机

每次讲到 C++ 核心语法，总会有人问：「老师，存储引擎为什么非得用 C++，用 Java 或者 Go 不行吗？」我们的回答很直接——在 MemTable 这种纳秒级热路径上，GC 的停顿、运行时的不确定性、字节码的解释开销，都会成为系统抖动的来源。C++ 给你的不是性能更快，而是性能可控：什么时候分配、什么时候释放、有没有拷贝，全都摆在你眼前。LevelDB、RocksDB、Redis 都是这套逻辑，TitanKV 也是顺着这条路走。

这一模块在整个 TitanKV 课程里是「底层语言地基」的位置：Module 01 给你看了全局，Module 02 就带你下到 C++ 这块地基上，把 Slice、Status、varint 这些工业界惯用法一个一个掰开揉碎。后面 Module 03 讲现代 C++ 与并发、Module 05 讲跳表实现，都会反复用到这里的 Slice 比较、Status 返回、varint 编码，所以我们在这里把基础打实，后面读源码才不会卡壳。哪怕只是理解 `Status` 的错误码返回方式，也是看懂 `DBImpl::Get` 流程的前提。

学完之后，你应该能讲清这几个面试高频题：为什么 LevelDB 风格的 Slice 不持有内存、为什么不抛异常而是返回 Status、varint 编码为什么每字节只用 7 位。更重要的是，下次你看 minikv 源码时，不会因为 `Slice(const char*, size_t)` 这种构造函数而发怵，因为你已经知道它背后那套「零拷贝视图」的设计哲学。走完这一节，你会发现自己看 LevelDB、RocksDB 源码时也轻松了一大截。

## 1. 核心知识

- C++ 编译模型：头文件（声明）+ 源文件（定义），`#include` 是文本粘贴，`#pragma once` 防重复包含。
- 值类别：lvalue / rvalue / xvalue；`const` 与引用折叠。
- 指针 vs 引用：指针可空、可重指向、有多级；引用必须初始化、不可重绑定。
- 函数重载、命名空间、`enum class` 强类型枚举。
- 项目惯用法：`Slice`（不持有内存的轻量视图）、`Status`（错误码 + 消息）、`varint` 编码。
- RAII 雏形与五法则。

## 2. 内容详解

### 2.1 头文件与编译模型

C++ 采用分离编译：声明放头文件，定义放源文件。`#include` 是预处理期的**文本粘贴**，因此头文件被多次包含会重复定义，需用 `#pragma once` 或 include guard 防护。

minikv 全部头文件以 `#pragma once` 开头（见 [slice.h:1](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/slice.h)）。命名空间用 `namespace minikv { ... }` 防止符号冲突，工具函数再套一层 `namespace utils`（见 [coding.h:6-7](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/coding.h)）。

理解分离编译最直观的方式是看一条 `.cpp` 文件是怎么变成可执行文件的——预处理、编译、汇编、链接四步，每一步产物不同、看的对象不同：

```mermaid
flowchart LR
    Src["*.h + *.cpp<br/>源码"] --> PP["预处理<br/>#include 展开 / 宏替换"]
    PP --> Comp["编译<br/>语法分析 + 优化"]
    Comp --> Asm["汇编<br/>生成 .o 目标文件"]
    Asm --> Link["链接<br/>合并静态库 / 解析符号"]
    Link --> Bin["可执行文件<br/>minikv_test"]
```

`#pragma once` 在预处理这一步生效——预处理器看到同一头文件第二次被包含时直接跳过，避免重复定义；链接这一步则把 `minikv` 静态库和测试代码合到一起，最终得到 `ctest` 跑的可执行文件。

### 2.2 Slice —— 轻量字符串视图

[slice.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/slice.h) 实现了一个 LevelDB 风格的 `Slice`：

```cpp
class Slice {
public:
    Slice() : data_(""), size_(0) {}
    Slice(const char* d) : data_(d), size_(d ? std::strlen(d) : 0) {}
    Slice(const char* d, size_t n) : data_(d), size_(n) {}
    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}
    Slice(std::string_view sv) : data_(sv.data()), size_(sv.size()) {}
    // ...
private:
    const char* data_;
    size_t size_;
};
```

要点：

- **不持有内存**：`data_` 指向外部缓冲区，`Slice` 析构不释放。这避免了 `std::string` 的堆分配，是热路径上的关键优化。
- **多个构造函数重载**：接受 `const char*`、`const std::string&`、`std::string_view`，体现函数重载。
- **`const` 成员函数**：`data()`/`size()` 等都是 `const`，保证 const 对象可调用。
- **运算符重载**：`operator[]`、`operator==` 等，使 `Slice` 用起来像内置类型。
- **陷阱**：`Slice` 持有的指针若指向临时对象，悬空引用即未定义行为。

### 2.3 Status —— 错误处理惯用法

[status.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/status.h) 用 `Status` 替代异常：

```cpp
enum class StatusCode {  // 强类型枚举，不会隐式转 int
    kOk = 0, kNotFound = 1, kCorruption = 2, kNotSupported = 3,
    kInvalidArgument = 4, kIOError = 5,
};

class Status {
public:
    static Status Ok() { return Status(); }
    static Status NotFound(std::string msg = "") { ... }
    // ...
private:
    StatusCode code_;
    std::string msg_;
};
```

要点：

- **`enum class`**：C++11 引入，作用域受限、不会污染外层命名空间、不会隐式转 `int`，比 `enum` 更安全。
- **静态工厂方法**：`Status::NotFound(...)` 比 `Status(StatusCode::kNotFound, ...)` 更可读，且默认参数 `msg = ""` 提供便捷。
- **`std::move(msg)`**：构造时移动字符串，避免深拷贝（移动语义在 Module 03 详讲）。
- **为什么不用异常**：存储引擎追求零开销、可预测，异常在错误路径有性能不确定性；Status 是显式错误传播。

### 2.4 Varint 变长编码

[coding.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/coding.h) 实现了 Protobuf 风格的变长整数编码：

```cpp
inline void encodeVariant32(std::string& dst, uint32_t val) {
    while (val >= 0x80) {
        dst.push_back(static_cast<char>(val | 0x80));  // 高位 1 表示还有后续字节
        val >>= 7;
    }
    dst.push_back(static_cast<char>(val));              // 高位 0 表示结束
}
```

原理：每字节用低 7 位存数据，最高位作「继续标志」。小数字（<128）只占 1 字节，节省磁盘空间。SSTable 的 key/value 长度、序列号都用 varint 编码。

`static_cast<char>` 显式转型避免 C 风格转换 `(char)` 的风险——`static_cast` 编译期检查更严格。

### 2.5 指针 vs 引用

| 维度 | 指针 `T*` | 引用 `T&` |
|---|---|---|
| 可空 | 是 | 否（必须初始化） |
| 可重指向 | 是 | 否 |
| 多级 | `T**` 合法 | `T&&` 是右值引用不是多级 |
| `sizeof` | 指针大小 | 所指对象大小（编译期） |
| 自增 | 移动指针 | 非法 |

项目里 `Slice::data_` 是 `const char*`（可空、可改指向），而 `Status(StatusCode code, std::string msg)` 形参用值传递（拷贝后再 move 进成员）。

### 2.6 RAII 与五法则

**RAII**（Resource Acquisition Is Initialization）：资源获取即初始化，对象生命周期绑定资源——构造获取、析构释放，即使抛异常栈展开也会调用析构。

minikv 的 `DBImpl` 持有 `std::unique_ptr<WAL>`、`std::unique_ptr<MemTable>`（见 [db_impl.h:39-42](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/db_impl.h)），`DBImpl` 析构时成员 unique_ptr 自动释放，无需手写析构释放逻辑。

**五法则**：若声明了析构/拷贝构造/拷贝赋值/移动构造/移动赋值中任一个，通常需全部声明。`Status` 只声明了构造，未声明拷贝/移动——编译器生成的默认版本即够用（零法则）。

## 3. 思考题

1. `Slice` 持有 `const char*` 但不持有内存，举一个会让 `Slice` 悬空的错误用法。
2. 为什么 `Status` 用 `enum class` 而不是 `enum`？写出一条 `enum` 会出错而 `enum class` 不会的代码。
3. `encodeVariant32` 中 `val | 0x80` 的 `0x80` 是十进制多少？为什么用按位或而不是加法？
4. `Slice(const char* d)` 构造函数里 `d ? std::strlen(d) : 0` 为什么要判空？传 `nullptr` 会怎样？
5. 函数返回 `Status` 用值返回（`return Status();`），会不会触发拷贝？编译器做了什么优化？

## 4. 动手题

### 题 4.1（手写 Slice）

不参考源码，实现一个最小 `Slice`：支持 `const char*` 和 `std::string` 构造、`size()`、`data()`、`operator==`、`startsWith`，并写 5 个单元测试。

### 题 4.2（Varint 编解码）

实现 `encodeVariant64` / `decodeVariant64`（64 位版本）。测试：编码 `0xFFFFFFFFFFFFFFFF`（最大值）需要几字节？为什么？

### 题 4.3（Status 实战）

参考 [status.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/status.h)，实现一个 `Result<T>` 模板：成功携带 `T`，失败携带 `Status`。要求支持 `map`/`andThen` 链式调用（类似 Rust 的 `Result`）。

## 5. 自检

1. `#pragma once` 的作用是________________。
2. `enum class` 相比 `enum` 的两个优势：______ 和 ______。
3. `Slice` 不持有内存意味着调用方必须保证所指向的缓冲区________。
4. varint 编码中，每字节的最高位是 1 表示________。
5. RAII 的全称是________________，核心思想是把________绑定到对象生命周期。

<details>
<summary>参考答案</summary>

1. 防止头文件被重复包含（同 include guard 作用但更简洁）
2. 作用域受限（不会污染外层命名空间）；不会隐式转换为 int
3. 在 Slice 生命周期内有效（否则悬空引用）
4. 还有后续字节（继续标志）
5. Resource Acquisition Is Initialization；资源获取与释放

思考题要点：
1. `Slice s(std::string("temp").c_str());` —— 临时 string 析构后 s 悬空。
2. `enum Color { Red, Green }; int x = Red;`（合法但危险）；`enum class Color { Red }; int x = Color::Red;`（编译错误）。
3. 128。用按位或保证只置最高位而不影响低 7 位；加法可能在低 7 位已置位时进位出错。
4. 防御性编程，`std::strlen(nullptr)` 是 UB。
5. NRVO（命名返回值优化）/ RVO 会消除拷贝；C++17 起对纯右值 guaranteed copy elision。

</details>

---

← [Module 01](./01-overview.md)  |  下一模块：[Module 03 — 现代 C++ 与并发](./03-modern-cpp.md) →
