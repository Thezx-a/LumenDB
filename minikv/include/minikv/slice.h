#pragma once
#include <cstring>
#include <string>
#include <string_view>

namespace minikv {

class Slice {
public:
Slice() : data_(""), size_(0) {}
    Slice(const char* d) : data_(d), size_(d ? std::strlen(d) : 0) {}
    Slice(const char* d, size_t n) : data_(d), size_(n) {}
    Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}
    Slice(std::string_view sv) : data_(sv.data()), size_(sv.size()) {}

    const char* data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    char operator[](size_t n) const { return data_[n]; }

    void clear() { data_ = ""; size_ = 0; }
    void remove_prefix(size_t n) {
        data_ += n;
        size_ -= n;
    }

    std::string toString() const { return std::string(data_, size_); }
    std::string_view toStringView() const { return std::string_view(data_, size_); }
    bool startsWith(const Slice& prefix) const {
        return size_ >= prefix.size_ &&
               std::memcmp(data_, prefix.data_, prefix.size_) == 0;
    }

    int compare(const Slice& b) const {
        const size_t min_len = size_ < b.size_ ? size_ : b.size_;
        int r = std::memcmp(data_, b.data_, min_len);
        if (r == 0) {
            if (size_ < b.size_) r = -1;
            else if (size_ > b.size_) r = +1;
        }
        return r;
    }

    bool operator==(const Slice& b) const { return compare(b) == 0; }
    bool operator!=(const Slice& b) const { return compare(b) != 0; }
    bool operator<(const Slice& b) const { return compare(b) < 0; }
    bool operator>(const Slice& b) const { return compare(b) > 0; }
    bool operator<=(const Slice& b) const { return compare(b) <= 0; }
    bool operator>=(const Slice& b) const { return compare(b) >= 0; }

private:
    const char* data_;
    size_t size_;
};

}  // namespace minikv
