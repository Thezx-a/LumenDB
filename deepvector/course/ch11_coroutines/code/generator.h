#pragma once

#include <coroutine>
#include <optional>
#include <utility>

template<typename T>
class Generator {
public:
    struct promise_type {
        T current_value_;
        std::exception_ptr exception_;

        Generator get_return_object() {
            return Generator{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            current_value_ = std::move(value);
            return {};
        }

        void return_void() {}
        void unhandled_exception() { exception_ = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> handle_;
    explicit Generator(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~Generator() { if (handle_) handle_.destroy(); }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& o) noexcept
        : handle_(std::exchange(o.handle_, nullptr)) {}

    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        std::coroutine_handle<promise_type> handle_;
        bool done_ = false;

        iterator() : handle_(nullptr), done_(true) {}
        explicit iterator(
            std::coroutine_handle<promise_type> h) : handle_(h) {
            if (handle_ && !handle_.done()) {
                handle_.resume();
                done_ = handle_.done();
            } else {
                done_ = true;
            }
        }

        reference operator*() { return handle_.promise().current_value_; }

        iterator& operator++() {
            handle_.resume();
            done_ = handle_.done();
            return *this;
        }

        bool operator==(const iterator& other) const {
            return done_ == other.done_;
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    iterator begin() { return iterator{handle_}; }
    iterator end() { return iterator{}; }
};
