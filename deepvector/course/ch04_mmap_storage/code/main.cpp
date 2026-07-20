#include "mmap_array.h"
#include <iostream>
#include <cassert>
#include <unistd.h>

int main() {
    const char* path = "test_float_array.bin";

    {
        std::cout << "=== Round 1: Writing ===" << std::endl;
        MmapFloatArray arr(path, 8);
        for (int i = 0; i < 10; i++) arr.push_back(i * 1.5f);
        std::cout << "Size: " << arr.size() << "  Capacity: " << arr.capacity() << std::endl;
    }

    {
        std::cout << "\n=== Round 2: Re-reading ===" << std::endl;
        MmapFloatArray arr(path);
        assert(arr.size() == 10);
        assert(arr.capacity() == 16);
        for (size_t i = 0; i < arr.size(); i++) {
            assert(arr.at(i) == i * 1.5f);
            std::cout << "  [" << i << "] = " << arr.at(i) << std::endl;
        }
    }

    unlink(path);
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
