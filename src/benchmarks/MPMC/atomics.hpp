#pragma once

#include <atomic>
#include <new>
#include <string_view>
#include <vector>

namespace benchmarks {
    
struct AtomicMPMCQueue {
    static constexpr std::string_view NAME = "Atomic MPMC queue";

    std::vector<int> data;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> read_idx{};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> reserved_read_idx{};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> write_idx{};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> reserved_write_idx{};

    // Zero initialise data
    AtomicMPMCQueue(size_t capacity) : data(capacity, 0) {}

    void reset() {
        read_idx.store(0, std::memory_order_relaxed);
        reserved_read_idx.store(0, std::memory_order_relaxed);
        write_idx.store(0, std::memory_order_relaxed);
        reserved_write_idx.store(0, std::memory_order_relaxed);
    }

    bool push(int val) {
        size_t reserved;
        size_t next;
        do {
            reserved = reserved_write_idx.load(std::memory_order_relaxed);
            next = (reserved + 1) % data.size();

            if (next == read_idx.load(std::memory_order_acquire)) {
                return false;  // We have a full queue
            }
        } while (
            !reserved_write_idx.compare_exchange_weak(reserved, next, std::memory_order_relaxed));
        // If reserved write idx matches our reservation, we can update it with next

        data[reserved] = val;

        // Reserved is now used up
        size_t expected = reserved;
        while (!write_idx.compare_exchange_weak(expected, next, std::memory_order_release,
                                                std::memory_order_relaxed)) {
            // On a failure the CAS instruction updates expected with the actual value, we want to
            // set that back
            expected = reserved;
        }

        return true;
    }

    bool pop(int& val) {
        size_t reserved;
        size_t next;
        do {
            reserved = reserved_read_idx.load(std::memory_order_relaxed);
            // Know this has already been written to
            if (reserved == write_idx.load(std::memory_order_acquire)) return false;

            next = (reserved + 1) % data.size();
        } while (
            !reserved_read_idx.compare_exchange_weak(reserved, next, std::memory_order_relaxed));

        val = data[reserved];

        size_t expected = reserved;
        while (!read_idx.compare_exchange_weak(expected, next, std::memory_order_release,
                                               std::memory_order_relaxed)) {
            expected = reserved;
        }
        return true;
    }
};

}  // namespace benchmarks