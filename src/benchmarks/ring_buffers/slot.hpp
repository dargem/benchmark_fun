#pragma once

#include <atomic>
#include <cassert>
#include <new>
#include <string_view>
#include <vector>

struct Slot {
    alignas(std::hardware_destructive_interference_size) std::atomic<bool> written;
    int data;
};

struct SlotRingBuffer {
    static constexpr std::string_view NAME = "Slot ring buffer";

    size_t mask;
    std::vector<Slot> data;
    // No need for atomics SPSC queue, readers and writers don't look at each others idx
    size_t read_idx{};
    size_t write_idx{};

    // Zero initialise data
    SlotRingBuffer(size_t capacity) : mask(capacity - 1), data(capacity) {
        assert((mask & capacity) == 0);
    }

    void reset() {
        read_idx = 0;
        write_idx = 0;

        // Capacity will be mask + 1
        for (size_t i{}; i < mask + 1; ++i) data[i].written = false;
    }

    bool push(int val) {
        Slot& slot = data[write_idx & mask];

        if (slot.written.load(std::memory_order_acquire)) {
            return false;  // Queue is full if slot already written to
        }

        // Now we can write to it
        slot.data = val;
        slot.written.store(true, std::memory_order_release);
        ++write_idx;
        return true;
    }

    bool pop(int& val) {
        Slot& slot = data[read_idx & mask];

        // Queue is empty if already read
        if (!slot.written.load(std::memory_order_acquire)) return false;

        val = slot.data;
        slot.written.store(false, std::memory_order_release);
        ++read_idx;
        return true;
    }
};