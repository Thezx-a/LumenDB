#include "dv/storage/vector_store.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace dv {
namespace storage {

static const uint64_t kFileMagic = 0x4C554D454E444220ULL; // "DEEPVECTOR"
static const uint64_t kInvalidID = 0;
static const size_t kHeaderSize = 64;

// On-disk header (64 bytes):
//   magic(8) + dim(4) + count(4) + capacity(4) + data_offset(4) + id_offset(4) + reserved(36)

static void ensureDir(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        std::string dir = path.substr(0, pos);
        ::mkdir(dir.c_str(), 0755);
    }
}

VectorStore::VectorStore(Dimension dim, const std::string& path)
    : dim_(dim), path_(path), fd_(-1), data_(nullptr), ids_(nullptr),
      capacity_(0), count_(0), file_size_(0), dirty_(false) {
    ensureDir(path);
    
    size_t init_cap = 1024;
    size_t vector_size = dim * sizeof(float);
    size_t id_size = init_cap * sizeof(uint64_t);
    size_t data_size = init_cap * vector_size;
    file_size_ = kHeaderSize + id_size + data_size;

    fd_ = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_ < 0) throw std::runtime_error("Cannot create vector store: " + path);
    ::ftruncate(fd_, file_size_);

    data_ = static_cast<float*>(::mmap(nullptr, file_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (data_ == MAP_FAILED) { ::close(fd_); throw std::runtime_error("mmap failed"); }

    char* raw = reinterpret_cast<char*>(data_);
    std::memcpy(raw, &kFileMagic, 8);
    std::memcpy(raw + 8, &dim_, 4);
    std::memcpy(raw + 12, &count_, 4);
    std::memcpy(raw + 16, &init_cap, 4);
    uint32_t data_off = kHeaderSize + static_cast<uint32_t>(id_size);
    uint32_t id_off = kHeaderSize;
    std::memcpy(raw + 20, &data_off, 4);
    std::memcpy(raw + 24, &id_off, 4);

    ids_ = reinterpret_cast<uint64_t*>(raw + id_off);
    capacity_ = init_cap;
    std::memset(ids_, 0, id_size);
}

VectorStore::VectorStore(Dimension dim, int fd)
    : dim_(dim), fd_(fd), data_(nullptr), ids_(nullptr),
      capacity_(0), count_(0), file_size_(0), dirty_(false) {}

VectorStore::~VectorStore() {
    if (data_ && data_ != MAP_FAILED) {
        flush();
        ::munmap(data_, file_size_);
    }
    if (fd_ >= 0) ::close(fd_);
}

void VectorStore::flush() {
    if (dirty_ && data_) {
        ::msync(data_, file_size_, MS_SYNC);
        dirty_ = false;
    }
}

uint64_t VectorStore::nextID() {
    for (size_t i = 0; i < capacity_; ++i) {
        if (ids_[i] == kInvalidID) return i + 1;
    }
    return capacity_ + 1;
}

void VectorStore::ensureCapacity(size_t needed) {
    if (needed <= capacity_) return;
    grow(needed * 2);
}

void VectorStore::grow(size_t new_capacity) {
    flush();
    size_t vector_size = dim_ * sizeof(float);
    size_t old_id_size = capacity_ * sizeof(uint64_t);
    size_t old_data_size = capacity_ * vector_size;
    size_t new_id_size = new_capacity * sizeof(uint64_t);
    size_t new_data_size = new_capacity * vector_size;
    size_t new_file_size = kHeaderSize + new_id_size + new_data_size;

    ::munmap(data_, file_size_);
    ::ftruncate(fd_, new_file_size);
    data_ = static_cast<float*>(::mmap(nullptr, new_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (data_ == MAP_FAILED) throw std::runtime_error("mmap grow failed");

    file_size_ = new_file_size;
    uint32_t new_id_off = kHeaderSize;
    uint32_t new_data_off = kHeaderSize + static_cast<uint32_t>(new_id_size);

    char* raw = reinterpret_cast<char*>(data_);
    std::memcpy(raw + 16, &new_capacity, 4);
    std::memcpy(raw + 20, &new_data_off, 4);
    std::memcpy(raw + 24, &new_id_off, 4);

    ids_ = reinterpret_cast<uint64_t*>(raw + new_id_off);
    capacity_ = new_capacity;
}

uint64_t VectorStore::append(const float* vector) {
    uint64_t id = nextID();
    size_t idx = id - 1;
    ensureCapacity(idx + 1);

    size_t vector_size = dim_ * sizeof(float);
    uint32_t data_off = kHeaderSize + static_cast<uint32_t>(capacity_ * sizeof(uint64_t));
    float* target = data_ + (data_off / sizeof(float)) + idx * dim_;
    std::memcpy(target, vector, vector_size);

    ids_[idx] = id;
    if (id > static_cast<uint64_t>(count_)) count_ = static_cast<size_t>(id);

    size_t c = count_;
    std::memcpy(reinterpret_cast<char*>(data_) + 12, &c, 4);
    dirty_ = true;
    return id;
}

const float* VectorStore::get(uint64_t id) const {
    if (id == kInvalidID || id > capacity_) return nullptr;
    size_t idx = id - 1;
    if (ids_[idx] != id) return nullptr;
    uint32_t data_off = kHeaderSize + static_cast<uint32_t>(capacity_ * sizeof(uint64_t));
    return data_ + (data_off / sizeof(float)) + idx * dim_;
}

void VectorStore::remove(uint64_t id) {
    if (id == kInvalidID || id > capacity_) return;
    size_t idx = id - 1;
    ids_[idx] = kInvalidID;
    if (count_ > 0) count_--;
    size_t c = count_;
    std::memcpy(reinterpret_cast<char*>(data_) + 12, &c, 4);
    dirty_ = true;
}

std::unique_ptr<VectorStore> VectorStore::load(const std::string& path) {
    int fd = ::open(path.c_str(), O_RDWR);
    if (fd < 0) return nullptr;

    struct stat st;
    ::fstat(fd, &st);
    size_t fsize = st.st_size;
    if (fsize < kHeaderSize) { ::close(fd); return nullptr; }

    char header[kHeaderSize];
    ::read(fd, header, kHeaderSize);

    uint64_t magic;
    std::memcpy(&magic, header, 8);
    if (magic != kFileMagic) { ::close(fd); return nullptr; }

    Dimension dim;
    uint32_t count, cap, data_off, id_off;
    std::memcpy(&dim, header + 8, 4);
    std::memcpy(&count, header + 12, 4);
    std::memcpy(&cap, header + 16, 4);
    std::memcpy(&data_off, header + 20, 4);
    std::memcpy(&id_off, header + 24, 4);

    auto store = std::unique_ptr<VectorStore>(new VectorStore(dim, fd));
    store->file_size_ = fsize;
    store->capacity_ = cap;
    store->count_ = count;

    store->data_ = static_cast<float*>(::mmap(nullptr, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (store->data_ == MAP_FAILED) { ::close(fd); return nullptr; }

    char* raw = reinterpret_cast<char*>(store->data_);
    store->ids_ = reinterpret_cast<uint64_t*>(raw + id_off);
    return store;
}

} // namespace storage
} // namespace dv
