#include "pimpl.h"
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <iostream>

class Widget::Impl {
public:
    void Set(const std::string& k, int v) {
        std::unique_lock lock(mutex_);
        int old_val = 0;
        auto it = data_.find(k);
        if (it != data_.end()) old_val = it->second;
        data_[k] = v;
        lock.unlock();
        fireCallbacks(k, old_val, v);
    }

    std::optional<int> Get(const std::string& k) const {
        std::shared_lock lock(mutex_);
        auto it = data_.find(k);
        if (it != data_.end()) return it->second;
        return std::nullopt;
    }

    size_t Size() const {
        std::shared_lock lock(mutex_);
        return data_.size();
    }

    void OnChange(ChangeCallback cb) {
        std::unique_lock lock(mutex_);
        callbacks_.push_back(std::move(cb));
    }

private:
    void fireCallbacks(const std::string& k, int old_v, int new_v) {
        std::shared_lock lock(mutex_);
        for (auto& cb : callbacks_) {
            cb(k, old_v, new_v);
        }
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, int> data_;
    std::vector<ChangeCallback> callbacks_;
};

Widget::Widget() : impl_(std::make_unique<Impl>()) {}
Widget::~Widget() = default;
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;

void Widget::Set(const std::string& k, int v) { impl_->Set(k, v); }
std::optional<int> Widget::Get(const std::string& k) const { return impl_->Get(k); }
size_t Widget::Size() const { return impl_->Size(); }
void Widget::OnChange(ChangeCallback cb) { impl_->OnChange(std::move(cb)); }
