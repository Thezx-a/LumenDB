# Module 02 — C++ Core Syntax

> Source: [slice.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/slice.h), [status.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/status.h), [coding.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/coding.h), [db.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/db.h)

## 1. Core Knowledge

- C++ compilation model: headers (declarations) + source files (definitions); `#include` is textual paste; `#pragma once` prevents multiple inclusion.
- Value categories: lvalue / rvalue / xvalue; `const` and reference collapsing.
- Pointers vs references: pointers can be null, re-pointed, multi-level; references must be initialized and cannot be re-bound.
- Function overloading, namespaces, `enum class` scoped enumerations.
- Project idioms: `Slice` (lightweight non-owning view), `Status` (code + message), varint encoding.
- RAII basics and the Rule of Five.

## 2. Deep Dive

### 2.1 Headers and the Compilation Model

C++ uses separate compilation: declarations go in headers, definitions in source files. `#include` is **textual paste** at preprocess time, so multiple inclusion causes redefinition — guard with `#pragma once` or include guards.

Every minikv header starts with `#pragma once` (see [slice.h:1](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/slice.h)). Namespaces `namespace minikv { ... }` prevent symbol collisions; utility functions add an inner `namespace utils` (see [coding.h:6-7](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/coding.h)).

### 2.2 Slice — A Lightweight String View

[slice.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/slice.h) implements a LevelDB-style `Slice`:

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

Key points:

- **Non-owning**: `data_` points to an external buffer; `Slice`'s destructor does not free it. This avoids `std::string` heap allocation — critical on hot paths.
- **Multiple overloaded constructors**: accept `const char*`, `const std::string&`, `std::string_view` — function overloading in action.
- **`const` member functions**: `data()`/`size()` etc. are `const`, so const objects can call them.
- **Operator overloading**: `operator[]`, `operator==`, etc. make `Slice` behave like a built-in type.
- **Pitfall**: if `Slice` holds a pointer to a temporary, the dangling reference is UB.

### 2.3 Status — The Error-Handling Idiom

[status.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/status.h) replaces exceptions with `Status`:

```cpp
enum class StatusCode {  // scoped enum, no implicit int conversion
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

Key points:

- **`enum class`**: introduced in C++11, scope-restricted, does not pollute the outer namespace, no implicit `int` conversion — safer than `enum`.
- **Static factory methods**: `Status::NotFound(...)` reads better than `Status(StatusCode::kNotFound, ...)`; default arg `msg = ""` adds convenience.
- **`std::move(msg)`**: moves the string at construction to avoid deep copy (move semantics covered in Module 03).
- **Why not exceptions**: storage engines want zero-overhead, predictable behavior; exceptions have nondeterministic cost on error paths. Status is explicit error propagation.

### 2.4 Varint Variable-Length Encoding

[coding.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/utils/coding.h) implements Protobuf-style varint encoding:

```cpp
inline void encodeVariant32(std::string& dst, uint32_t val) {
    while (val >= 0x80) {
        dst.push_back(static_cast<char>(val | 0x80));  // high bit 1 = more bytes follow
        val >>= 7;
    }
    dst.push_back(static_cast<char>(val));              // high bit 0 = done
}
```

Principle: each byte stores 7 data bits in the low bits; the high bit is a "continuation flag". Small numbers (<128) take 1 byte, saving disk space. SSTable key/value lengths and sequence numbers all use varint.

`static_cast<char>` is an explicit cast that avoids the risk of C-style `(char)` — `static_cast` does stricter compile-time checks.

### 2.5 Pointers vs References

| Aspect | Pointer `T*` | Reference `T&` |
|---|---|---|
| Nullable | yes | no (must be initialized) |
| Re-pointable | yes | no |
| Multi-level | `T**` legal | `T&&` is an rvalue reference, not multi-level |
| `sizeof` | pointer size | referred object size (compile time) |
| Increment | moves pointer | illegal |

In the project, `Slice::data_` is `const char*` (nullable, re-pointable), while `Status(StatusCode code, std::string msg)` takes parameters by value (copy then move into the member).

### 2.6 RAII and the Rule of Five

**RAII** (Resource Acquisition Is Initialization): bind resources to object lifetimes — acquire in the constructor, release in the destructor; even when an exception unwinds the stack, destructors run.

minikv's `DBImpl` holds `std::unique_ptr<WAL>`, `std::unique_ptr<MemTable>` (see [db_impl.h:39-42](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/db_impl.h)); when `DBImpl` is destroyed, the member unique_ptrs release automatically — no manual destructor logic needed.

**Rule of Five**: if you declare any of destructor / copy ctor / copy assign / move ctor / move assign, you usually need all five. `Status` declares only constructors, not copy/move — the compiler-generated defaults suffice (Rule of Zero).

## 3. Thinking Questions

1. `Slice` holds `const char*` but does not own memory. Give a usage that leaves `Slice` dangling.
2. Why does `Status` use `enum class` instead of `enum`? Write a line that compiles wrongly with `enum` but fails with `enum class`.
3. In `encodeVariant32`, what is `0x80` in decimal? Why bitwise-OR instead of addition?
4. Why does `Slice(const char* d)` check `d ? ... : 0`? What happens if you pass `nullptr`?
5. `Status` is returned by value (`return Status();`). Does this trigger a copy? What optimization does the compiler apply?

## 4. Hands-on Exercises

### Exercise 4.1 (Hand-write Slice)

Without looking at the source, implement a minimal `Slice`: constructors for `const char*` and `std::string`, `size()`, `data()`, `operator==`, `startsWith`, plus 5 unit tests.

### Exercise 4.2 (Varint Codec)

Implement `encodeVariant64` / `decodeVariant64` (64-bit versions). Test: how many bytes does encoding `0xFFFFFFFFFFFFFFFF` (max) take? Why?

### Exercise 4.3 (Status in Practice)

Following [status.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/include/minikv/status.h), implement a `Result<T>` template: success carries `T`, failure carries `Status`. Support `map` / `andThen` chaining (like Rust's `Result`).

## 5. Self-Check

1. `#pragma once` is used to ________________.
2. Two advantages of `enum class` over `enum`: ______ and ______.
3. `Slice` not owning memory means the caller must ensure the buffer ____________.
4. In varint encoding, a high bit of 1 means ____________.
5. RAII stands for ________________; the core idea is to bind ________ to object lifetime.

<details>
<summary>Reference Answers</summary>

1. prevent a header from being included multiple times (like an include guard, but cleaner)
2. scope-restricted (no namespace pollution); no implicit int conversion
3. stays valid for the Slice's lifetime (otherwise dangling reference)
4. more bytes follow (continuation flag)
5. Resource Acquisition Is Initialization; resource acquire and release

Thinking question key points:
1. `Slice s(std::string("temp").c_str());` — the temporary string dies, s dangles.
2. `enum Color { Red, Green }; int x = Red;` (legal but dangerous); `enum class Color { Red }; int x = Color::Red;` (compile error).
3. 128. Bitwise-OR sets only the high bit without touching the low 7 bits; addition could carry if low bits are set.
4. Defensive programming; `std::strlen(nullptr)` is UB.
5. NRVO (Named Return Value Optimization) / RVO elides the copy; since C++17, guaranteed copy elision for prvalues.

</details>

---

← [Module 01](./01-overview.md)  |  Next: [Module 03 — Modern C++ & Concurrency](./03-modern-cpp.md) →
