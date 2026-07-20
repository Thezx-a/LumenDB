#include <cstdio>
#include "task.h"
#include "generator.h"

Task<int> compute_value(int x) {
    co_return x * x;
}

Task<int> add_values(int a, int b) {
    int va = co_await compute_value(a);
    int vb = co_await compute_value(b);
    co_return va + vb;
}

Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        int tmp = a + b;
        a = b;
        b = tmp;
    }
}

Generator<int> range(int start, int end) {
    for (int i = start; i < end; i++) {
        co_yield i;
    }
}

int main() {
    std::printf("=== C++20 Coroutines Demo ===\n\n");

    Task<int> task = add_values(3, 4);
    int result = task.get();
    std::printf("Task: compute_value(3) + compute_value(4) = %d\n", result);

    std::printf("\nFibonacci (first 15):\n");
    auto fib = fibonacci();
    int count = 0;
    for (int v : fib) {
        std::printf("  %d", v);
        if (++count >= 15) break;
    }
    std::printf("\n");

    std::printf("\nRange [10, 20):\n");
    for (int v : range(10, 20)) {
        std::printf("  %d", v);
    }
    std::printf("\n");

    std::printf("\nAll coroutines completed successfully.\n");
    return 0;
}
