#include "mmap_array.h"
#include <iostream>
#include <cstdlib>

void MmapFloatArray::init_file(size_t capacity) {
    file_size = data_offset() + capacity * sizeof(float);
    ftruncate(fd, file_size);
    ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap init"); exit(1); }
    header = reinterpret_cast<Header*>(ptr);
    header->magic = Header::MAGIC;
    header->version = Header::VERSION;
    header->element_size = sizeof(float);
    header->count = 0;
    header->capacity = capacity;
}

void MmapFloatArray::load_existing() {
    struct stat st;
    fstat(fd, &st);
    file_size = st.st_size;
    ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap load"); exit(1); }
    header = reinterpret_cast<Header*>(ptr);
    if (header->magic != Header::MAGIC) {
        std::cerr << "Error: Bad magic number!" << std::endl;
        exit(1);
    }
}

MmapFloatArray::MmapFloatArray(const char* path, size_t capacity) {
    fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) { perror("open"); exit(1); }
    struct stat st;
    fstat(fd, &st);
    if (st.st_size == 0) init_file(capacity);
    else load_existing();
}

MmapFloatArray::~MmapFloatArray() {
    msync(ptr, file_size, MS_SYNC);
    munmap(ptr, file_size);
    close(fd);
}

void MmapFloatArray::push_back(float val) {
    if (header->count >= header->capacity)
        grow(header->capacity * 2);
    float* data = reinterpret_cast<float*>(
        reinterpret_cast<char*>(ptr) + data_offset());
    data[header->count++] = val;
}

float MmapFloatArray::at(size_t i) const {
    float* data = reinterpret_cast<float*>(
        reinterpret_cast<char*>(ptr) + data_offset());
    return data[i];
}

size_t MmapFloatArray::size() const { return header->count; }
size_t MmapFloatArray::capacity() const { return header->capacity; }

void MmapFloatArray::grow(size_t new_capacity) {
    size_t new_file_size = data_offset() + new_capacity * sizeof(float);
    msync(ptr, file_size, MS_SYNC);
    munmap(ptr, file_size);
    ftruncate(fd, new_file_size);
    ptr = mmap(NULL, new_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap grow"); exit(1); }
    header = reinterpret_cast<Header*>(ptr);
    header->capacity = new_capacity;
    file_size = new_file_size;
}
