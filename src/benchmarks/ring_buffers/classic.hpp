#pragma once

#include <atomic>
#include <new>
#include <string>
#include <vector>

struct RingBuffer {
    std::string name = "Classic ring buffer";

    std::vector<int> data;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> read_idx{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> write_idx{0};

    // Zero initialise data
    RingBuffer(size_t capacity) : data(capacity, 0) {}

    bool push(int val) {
        const auto current_write_idx = write_idx.load(std::memory_order_relaxed);
        auto next_write_idx = current_write_idx + 1;
        if (next_write_idx == data.size()) {
            next_write_idx = 0;
        }

        if (next_write_idx == read_idx.load(std::memory_order_acquire)) {
            // read_idx is what we're reading next
            // One item is left unused to indicate the queue is full
            // So when write idx is 1 behind read idx we know its a full queue
            // Else if read_idx == write_idx it could be either full or empty
            return false;
        }

        data[current_write_idx] = val;
        write_idx.store(next_write_idx, std::memory_order_release);
        return true;
    }

    bool pop(int& val) {
        auto const current_read_idx = read_idx.load(std::memory_order_relaxed);
        if (current_read_idx == write_idx.load(std::memory_order_acquire)) {
            // We are reading to write_idx next so the queue is empty here
            return false;
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