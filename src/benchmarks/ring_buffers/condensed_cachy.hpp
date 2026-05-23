#pragma once

#include <atomic>
#include <new>
#include <vector>

struct CachingRingBufferCompressed {
    std::vector<int> data;

    // A mild modification to the data layout to save space. It think it will decrease performance
    // but I'm interested in how much. Now when say the popper hits its cache and needs to check the
    // write_idx the read_idx_cache will be on the same cacheline so they both get moved into
    // shared state. If the writer needs to write to the write_idx it will have to get exclusive
    // control of the cacheline to move it to modified. This is the same in both cases. If the
    // writer just is reading the read cache then it being in a shared state doesn't matter. In the
    // case the read cache needs updating the writer will need to additionally take control of the
    // write_data cache line. If it doesn't take control now it will in the next loop anyways since
    // it writes to it every loop so I theorize performance loss would be minor.
    alignas(std::hardware_destructive_interference_size) struct {
        std::atomic<size_t> read_idx{0};
        size_t write_idx_cache{0};
    } read_data;

    alignas(std::hardware_destructive_interference_size) struct {
        std::atomic<size_t> write_idx{0};
        size_t read_idx_cache{0};
    } write_data;

    // Zero initialise data
    CachingRingBufferCompressed(size_t capacity) : data(capacity, 0) {}

    bool push(int val) {
        const auto current_write_idx = write_data.write_idx.load(std::memory_order_relaxed);
        auto next_write_idx = current_write_idx + 1;
        if (next_write_idx == data.size()) {
            next_write_idx = 0;
        }

        // We check the cache first to avoid the scenario where the read_idx cacheline is in
        // modified state from another core. Then we want to read it meaning it has to move to a
        // shared state. This requires a cache to cache transfer which is slow. Now if possible we
        // just read our own cache to minimize the amount we access the data other cores are using.
        if (next_write_idx == write_data.read_idx_cache) {
            write_data.read_idx_cache = read_data.read_idx.load(std::memory_order_acquire);
            if (next_write_idx == write_data.read_idx_cache) {
                // read_idx is what we're reading next
                // One item is left unused to indicate the queue is full
                // So when write idx is 1 behind read idx we know its a full queue
                // Else if read_idx == write_idx it could be either full or empty
                return false;
            }
        }

        data[current_write_idx] = val;
        write_data.write_idx.store(next_write_idx, std::memory_order_release);
        return true;
    }

    bool pop(int& val) {
        auto const current_read_idx = read_data.read_idx.load(std::memory_order_relaxed);

        // Prefer checking with our write idx cache
        if (current_read_idx == read_data.write_idx_cache) {
            read_data.write_idx_cache = write_data.write_idx.load(std::memory_order_acquire);
            if (current_read_idx == read_data.write_idx_cache) {
                // We are reading to write_idx next so the queue is empty here
                return false;
            }
        }

        val = data[current_read_idx];
        auto next_read_idx = current_read_idx + 1;
        if (next_read_idx == data.size()) {
            next_read_idx = 0;
        }

        read_data.read_idx.store(next_read_idx, std::memory_order_release);
        return true;
    }
};