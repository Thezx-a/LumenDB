#include "skynet/core/timer_wheel.h"
#include <algorithm>

namespace skynet {
namespace core {

TimerWheel::TimerWheel(size_t wheel_size, uint64_t tick_ms)
    : wheel_size_(wheel_size), tick_ms_(tick_ms), current_tick_(0), next_id_(1) {
    wheel_.resize(wheel_size);
}

uint64_t TimerWheel::addTimer(uint64_t timeout_ms, std::coroutine_handle<> coro) {
    uint64_t total_ticks = (timeout_ms + tick_ms_ - 1) / tick_ms_;
    if (total_ticks == 0) total_ticks = 1;
    uint64_t expire_tick = current_tick_ + total_ticks;
    uint64_t id = next_id_++;
    TimerEntry entry{expire_tick, coro, id, false};
    all_entries_.push_back(entry);
    wheel_[slotFor(expire_tick)].push_back(entry);
    return id;
}

void TimerWheel::cancelTimer(uint64_t id) {
    for (auto& e : all_entries_) {
        if (e.id == id) { e.canceled = true; break; }
    }
}

void TimerWheel::advance(uint64_t elapsed_ms) {
    uint64_t ticks = (elapsed_ms + tick_ms_ - 1) / tick_ms_;
    current_tick_ += ticks;
}

std::vector<std::coroutine_handle<>> TimerWheel::getExpired() {
    for (auto& bucket : wheel_) {
        bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
            [this](const TimerEntry& e) {
                return e.expire_tick <= current_tick_ || e.canceled;
            }), bucket.end());
    }
    std::vector<std::coroutine_handle<>> result;
    for (auto it = all_entries_.begin(); it != all_entries_.end();) {
        if (it->expire_tick <= current_tick_ || it->canceled) {
            if (!it->canceled) result.push_back(it->coro);
            it = all_entries_.erase(it);
        } else {
            ++it;
        }
    }
    return result;
}

}  // namespace core
}  // namespace skynet
