#pragma once

#include <atomic>
#include <new>
#include <string_view>
#include <vector>

struct CachingRingBuffer {
    static constexpr std::string_view NAME = "Caching ring buffer";

    std::vector<int> data;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> read_idx{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> write_idx{0};
    // Add caches for read and write idx to minimize the amount of cross core communication
    alignas(std::hardware_destructive_interference_size) size_t read_idx_cache{0};
    alignas(std::hardware_destructive_interference_size) size_t write_idx_cache{0};

    // Zero initialise data
    CachingRingBuffer(size_t capacity) : data(capacity, 0) {}

    bool push(int val) {
        const auto current_write_idx = write_idx.load(std::memory_order_relaxed);
        auto next_write_idx = current_write_idx + 1;
        if (next_write_idx == data.size()) {
            next_write_idx = 0;
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