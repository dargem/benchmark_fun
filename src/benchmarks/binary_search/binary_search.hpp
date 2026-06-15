#pragma once

#include <sys/types.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "benchmarks/benchable.hpp"
#include "xoroshiro_64_star.hpp"

namespace benchmarks {

enum class CachePolicy { STANDARD, EYTZINGER };

template <size_t NUM_ELEMENTS, size_t NUM_SEARCHED, CachePolicy C>
    requires(NUM_SEARCHED < NUM_ELEMENTS && NUM_ELEMENTS <= 100000)
class StandardBinarySearch : public Benchable {
   public:
    StandardBinarySearch() : Benchable("Standard binary search") {
        if constexpr (C == CachePolicy::STANDARD) {
            rng.fill_aligned_uint32(data.data(), NUM_ELEMENTS);
            std::sort(data.begin(), data.end());
        } else if constexpr (C == CachePolicy::EYTZINGER) {
            std::vector<uint32_t> v;
            v.reserve(NUM_ELEMENTS + 1);
        }

        refillSearch();
    }

    void resetBenchmark() override { refillSearch(); }

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            for (uint32_t e : search) {
                sink = find(e);
            }
        }
    }

   private:
    volatile uint32_t sink;

    uint32_t find(uint32_t e) {
        if constexpr (C == CachePolicy::STANDARD) {
            return *std::lower_bound(data.begin(), data.end(), e);
        } else if constexpr (C == CachePolicy::EYTZINGER) {
        }
    }

    void refillSearch() {
        rng.fill_aligned_uint32(search.begin(), NUM_SEARCHED);
        std::for_each(search.begin(), search.end(), [&](uint32_t& e) {
            size_t idx = e % NUM_ELEMENTS;
            e = data[idx];
        });
    }

    // Reserve +1 space for eytzinger as it skips the first element
    alignas(std::max(utils::XoroshiroRNG::REGISTER_BYTE_SIZE,
                     64uz)) std::array<uint32_t, NUM_ELEMENTS + (C == CachePolicy::EYTZINGER)> data;
    std::array<uint32_t, NUM_SEARCHED> search;
    utils::XoroshiroRNG rng = utils::XoroshiroRNG();
};

}  // namespace benchmarks