#pragma once
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace skynet {
namespace core {

struct TimerEntry {
    uint64_t expire_tick;
    std::coroutine_handle<> coro;
    uint64_t id;
    bool canceled;
};

class TimerWheel {
public:
    explicit TimerWheel(size_t wheel_size = 1024, uint64_t tick_ms = 1);
    uint64_t addTimer(uint64_t timeout_ms, std::coroutine_handle<> coro);
    void cancelTimer(uint64_t id);
    void advance(uint64_t elapsed_ms);
    std::vector<std::coroutine_handle<>> getExpired();

private:
    std::vector<std::vector<TimerEntry>> wheel_;
    size_t wheel_size_;
    uint64_t tick_ms_;
    uint64_t current_tick_;
    uint64_t next_id_;
    std::vector<TimerEntry> all_entries_;

    size_t slotFor(uint64_t expire_tick) const {
        return expire_tick % wheel_size_;
    }
};

}  // namespace core
}  // namespace skynet
