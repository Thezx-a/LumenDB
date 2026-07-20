#include "VectorStorage.h"
#include <fstream>
#include <cstring>
#include <stdexcept>

VectorStorage::VectorStorage(size_t dimension, size_t capacity)
    : dimension_(dimension) {
    data_.reserve(capacity * dimension);
}

int64_t VectorStorage::append(const float* vector) {
    data_.insert(data_.end(), vector, vector + dimension_);
    return ++count_;
}

const float* VectorStorage::get_vector(int64_t id) const {
    if (id < 1 || id > count_) return nullptr;
    return data_.data() + (id - 1) * dimension_;
}

int64_t VectorStorage::count() const { return count_; }
size_t VectorStorage::dimension() const { return dimension_; }

void VectorStorage::save(const std::string& path) const {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Cannot open file for writing");

    uint32_t dim = static_cast<uint32_t>(dimension_);
    int64_t n = count_;
    ofs.write(reinterpret_cast<const char*>(&dim), sizeof(dim));
    ofs.write(reinterpret_cast<const char*>(&n), sizeof(n));
    ofs.write(reinterpret_cast<const char*>(data_.data()),
              data_.size() * sizeof(float));
}

VectorStorage VectorStorage::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open file for reading");

    uint32_t dim;
    int64_t n;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    ifs.read(reinterpret_cast<char*>(&n), sizeof(n));

    VectorStorage vs(dim, n);
    vs.count_ = n;
    vs.data_.resize(n * dim);
    ifs.read(reinterpret_cast<char*>(vs.data_.data()),
             n * dim * sizeof(float));
    return vs;
}
