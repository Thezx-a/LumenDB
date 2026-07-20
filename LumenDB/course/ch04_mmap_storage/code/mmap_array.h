#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <cstddef>

struct Header {
    uint32_t magic;
    uint32_t version;
    uint64_t element_size;
    uint64_t count;
    uint64_t capacity;
    uint8_t reserved[40];

    static constexpr uint32_t MAGIC = 0x4C4D4442;
    static constexpr uint32_t VERSION = 1;
};

class MmapFloatArray {
    int fd;
    void* ptr;
    size_t file_size;
    Header* header;

    size_t data_offset() const {
        return (sizeof(Header) + 63) & ~63ULL;
    }

    void init_file(size_t capacity);
    void load_existing();

public:
    MmapFloatArray(const char* path, size_t capacity = 1024);
    ~MmapFloatArray();

    void push_back(float val);
    float at(size_t i) const;
    size_t size() const;
    size_t capacity() const;
    void grow(size_t new_capacity);
};
