#pragma once

#include <atomic>
#include <cassert>
#include <new>
#include <string_view>
#include <vector>

struct Slot {
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> seq;
    int data;
};

struct SlotRingBuffer {
    static constexpr std::string_view NAME = "Slot ring buffer";

    size_t mask;
    std::vector<Slot> data;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> read_idx{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> write_idx{0};

    // Zero initialise data
    SlotRingBuffer(size_t capacity) : mask(capacity - 1), data(capacity) {
        assert((mask & capacity) == 0);
        for (size_t i{}; i < capacity; ++i) data[i].seq = i;
    }

    void reset() {
        read_idx.store(0, std::memory_order_relaxed);
        write_idx.store(0, std::memory_order_relaxed);
    }

    bool push(int val) {
        const auto current_write_idx = write_idx.load(std::memory_order_relaxed);
        auto next_write_idx = current_write_idx + 1;
        if (next_write_idx == data.size()) {
            next_write_idx = 0;
        }

        while (true) {
            
        }
        // We check the cache first to avoid the scenario where the read_idx cacheline is in
        // modified state from another core. Then we want to read it meaning it has to move to a
        // shared state. This requires a cache to cache transfer which is slow. Now if possible we
        // just read our own cache to minimize the amount we access the data other cores are using.
        if (next_write_idx == read_idx_cache) {
            read_idx_cache = read_idx.load(std::memory_order_acquire);
            if (next_write_idx == read_idx_cache) {
                // read_idx is what we're reading next
                // One item is left unused to indicate the queue is full
                // So when write idx is 1 behind read idx we know its a full queue
                // Else if read_idx == write_idx it could be either full or empty
                return false;
            }
        }

        data[current_write_idx] = val;
        write_idx.store(next_write_idx, std::memory_order_release);
        return true;
    }

    bool pop(int& val) {
        auto const current_read_idx = read_idx.load(std::memory_order_relaxed);

        // Prefer checking with our write idx cache
        if (current_read_idx == write_idx_cache) {
            write_idx_cache = write_idx.load(std::memory_order_acquire);
            if (current_read_idx == write_idx_cache) {
                // We are reading to write_idx next so the queue is empty here
                return false;
            }
        }

        val = data[current_read_idx];
        auto next_read_idx = current_read_idx + 1;
        if (next_read_idx == data.size()) {
            next_read_idx = 0;
        }

        read_idx.store(next_read_idx, std::memory_order_release);
        return true;
    }
};