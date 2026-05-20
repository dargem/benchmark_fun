#pragma once

#include <sys/types.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>

#include "benchmarks/benchable.hpp"
#include "xoroshiro_64_star.hpp"

namespace benchmarks {

template <size_t NUM_ELEMENTS, size_t NUM_SEARCHED>
    requires(NUM_SEARCHED < NUM_ELEMENTS && NUM_ELEMENTS <= 100000)
class StandardBinarySearch : public Benchable {
   public:
    StandardBinarySearch() : Benchable("Standard binary search") {
        rng.fill_aligned_uint32(data.data(), NUM_ELEMENTS);
        std::sort(data.begin(), data.end());
        refillSearch();
    }

    void resetBenchmark() override { refillSearch(); }

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            for (uint32_t e : search) {
                sink = standardFind(e);
                std::cout << e << '\n';
                std::cout << *(standardFind(e)) << '\n';
            }
        }
    }

    uint32_t* standardFind(uint32_t e) { return std::lower_bound(data.begin(), data.end(), e); }

   private:
    volatile uint32_t* sink;

    void refillSearch() {
        rng.fill_aligned_uint32(search.begin(), NUM_SEARCHED);
        std::for_each(search.begin(), search.end(), [&](uint32_t& e) {
            size_t idx = e % data.size();
            e = data[idx];
        });
    }

    alignas(XoroshiroRNG::REGISTER_BYTE_SIZE) std::array<uint32_t, NUM_ELEMENTS> data;
    std::array<uint32_t, NUM_SEARCHED> search;
    XoroshiroRNG rng = XoroshiroRNG();
};

}  // namespace benchmarks