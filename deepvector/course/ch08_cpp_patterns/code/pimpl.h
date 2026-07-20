#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>

class Widget {
public:
    Widget();
    ~Widget();
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    void Set(const std::string& k, int v);
    std::optional<int> Get(const std::string& k) const;

    size_t Size() const;

    using ChangeCallback = std::function<void(const std::string& key, int old_val, int new_val)>;
    void OnChange(ChangeCallback cb);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
