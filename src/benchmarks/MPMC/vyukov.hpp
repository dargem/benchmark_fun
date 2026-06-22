#pragma once
#include <atomic>
#include <cassert>
#include <string_view>
#include <vector>

namespace benchmarks {
struct Slot {
    alignas(64) std::atomic<size_t> sequence{0};
    int value;
};

class VyukovAtomicQueue {
   public:
    VyukovAtomicQueue(size_t size) : slots(size), mask(size - 1) {
        assert((size & mask) == 0);  // must be power of two
        for (size_t i = 0; i < size; ++i) slots[i].sequence.store(i, std::memory_order_relaxed);
    }

    bool push(int val) {
        size_t pos = write_idx.load(std::memory_order_relaxed);
        for (;;) {
            Slot& slot = slots[pos & mask];
            size_t seq = slot.sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                // Slot is ready — try to claim it
                if (write_idx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
                return false;  // full
            } else {
                pos = write_idx.load(std::memory_order_relaxed);
            }
        }
        slots[pos & mask].value = val;
        slots[pos & mask].sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool pop(int& val) {
        size_t pos = read_idx.load(std::memory_order_relaxed);
        for (;;) {
            Slot& slot = slots[pos & mask];
            size_t seq = slot.sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

            if (diff == 0) {
                if (read_idx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
                return false;  // empty
            } else {
                pos = read_idx.load(std::memory_order_relaxed);
            }
        }
        val = slots[pos & mask].value;
        slots[pos & mask].sequence.store(pos + mask + 1, std::memory_order_release);
        return true;
    }

    static constexpr std::string_view NAME = "Vyukov MPMC";

    void reset() {}

   private:
    std::vector<Slot> slots;
    size_t mask;
    alignas(64) std::atomic<size_t> write_idx{0};
    alignas(64) std::atomic<size_t> read_idx{0};
};

}  // namespace benchmarks