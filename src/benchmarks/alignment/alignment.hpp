#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>

#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

template <size_t AccessIndex>
    requires(AccessIndex < 64)  // Sanity check
class ArrayWrite : public Benchable {
   public:
    ArrayWrite() :
            Benchable(std::format(
                "{}{} array write, byte offset {}", AccessIndex % 8 == 0 ? "aligned" : "unaligned",
                AccessIndex > 56 ? " and crossing cacheline boundaries" : "", AccessIndex)) {}

    void resetBenchmark() override { bytes.fill(std::byte(1)); }

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            // For this bench we are basically writing 64 bit numbers at our access index
            uint64_t num = i;
            // kinda a stupid way to do it but avoid undefined behavior from breaking aliasing rules
            // the compiler will optimize away the temporary object anyways
            asm volatile("" : : "r"(bytes.data()) : "memory");
            std::memcpy(bytes.data() + AccessIndex, &num, sizeof(decltype(num)));
        }
    }

   private:
    // We 64 byte align this so we know its going to start at a cache line boundary
    alignas(64) std::array<std::byte, 128> bytes;
};

}  // namespace benchmarks