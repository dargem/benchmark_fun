#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string_view>

#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

// Credits to authors David Blackman and Sebastiano Vigna
// https://xorshift.di.unimi.it/xoroshiro128plus.c
class Xoroshiro128plus : public Benchable {
   public:
    // creates and seeds a generator
    explicit Xoroshiro128plus(uint64_t seed) : Benchable(std::string(NAME)) {
        // splitmix64 modifies the seed so this is deterministic with 2 different modified numbers
        state[0] = splitmix64(seed);
        state[1] = splitmix64(seed);
    }

    void runBenchmark(uint64_t iterations) override {
        for (uint64_t i{}; i < iterations; ++i) {
            sink = getRandomUint();
        }
    }

    void resetBenchmark() override { jump(); }

   private:
    volatile double sink{};

    /**
     * @brief creates a random uint64_t
     *
     * @return double
     */
    double getRandomUint() {
        const uint64_t state0 = state[0];
        uint64_t state1 = state[1];

        // create random number using the internal state
        const uint64_t result = state0 + state1;

        // mix up the internal state again
        state1 ^= state0;
        state[0] = rotl(state0, 24) ^ state1 ^ (state1 << 16);  // a, b
        state[1] = rotl(state1, 37);                            // c

        return result;
    }

    /**
     * @brief creates a random uniform double between [0,1)
     *
     * @return double
     */
    double getRandomDouble() {
        const uint64_t state0 = state[0];
        uint64_t state1 = state[1];

        // create random number using the internal state
        const uint64_t result = state0 + state1;

        // mix up the internal state again
        state1 ^= state0;
        state[0] = rotl(state0, 24) ^ state1 ^ (state1 << 16);  // a, b
        state[1] = rotl(state1, 37);                            // c

        // return, shifting result into [0,1)
        return (result >> 11) * (1.0 / (1ULL << 53));
    }

    // used internally for bitshifting an int
    static uint64_t rotl(uint64_t value, int shift) {
        return (value << shift) | (value >> (64 - shift));
    };

    static uint64_t splitmix64(uint64_t& seed) {
        uint64_t result = (seed += 0x9E3779B97F4A7C15ULL);
        result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ULL;
        result = (result ^ (result >> 27)) * 0x94D049BB133111EBULL;
        return result ^ (result >> 31);
    };

    /* This is the jump function for the generator. It is equivalent
    to 2^64 calls to next(); it can be used to generate 2^64
    non-overlapping subsequences for parallel computations. */

    void jump() {
        static const std::array<uint64_t, 2> JUMP = {0xdf900294d8f554a5, 0x170865df4b3201fc};

        uint64_t state0 = 0;
        uint64_t state1 = 0;
        for (const auto& jump_val : JUMP) {
            for (int bit_idx = 0; bit_idx < 64; bit_idx++) {
                if ((jump_val & (UINT64_C(1) << bit_idx)) != 0) {
                    state0 ^= state[0];
                    state1 ^= state[1];
                }
                std::ignore = getRandomUint();
            }
        }

        state[0] = state0;
        state[1] = state1;
    }

    // used to store internal state of RNG
    std::array<uint64_t, 2> state;
    constexpr static std::string_view NAME{"Xoroshiro128+ RNG"};
};

}  // namespace benchmarks