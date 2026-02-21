#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string_view>

#include "src/benchmarks/bench_types.hpp"
#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

template <bool isObservable>
class Xoroshiro128plus : public Benchable {
   public:
    // creates and seeds a generator
    explicit Xoroshiro128plus(size_t seed) :
            Benchable(BenchType::RANDOM_NUMBER_GENERATION,
                      isObservable ? OBSERVABLE_NAME : UNOBSERVABLE_NAME) {
        // splitmix64 modifies the seed so this is deterministic with 2 different modified numbers
        state[0] = splitmix64(seed);
        state[1] = splitmix64(seed);
    }

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            // shouldn't optimise out because internal state is being changed with
            // getRandomDouble(); but what if printNextDouble() isn't called? would it optimise it
            // out then?
            getRandomDouble();
        }
    }

    void resetBenchmark() override { jump(); }

    void printNextDouble() {
        if constexpr (isObservable) {
            // just in case so its impossible to prove that internal state doesn't matter
            std::cout << getRandomDouble() << std::endl;
        }
    }

   private:
    /**
     * @brief creates a random uniform double between [0,1)
     *
     * @return double
     */
    double getRandomDouble() {
        const size_t state0 = state[0];
        size_t state1 = state[1];

        // create random number using the internal state
        const size_t result = state0 + state1;

        // mix up the internal state again
        state1 ^= state0;
        state[0] = rotl(state0, 24) ^ state1 ^ (state1 << 16);  // a, b
        state[1] = rotl(state1, 37);                            // c

        // return, shifting result into [0,1)
        return (result >> 11) * (1.0 / (1ULL << 53));
    }

    // used internally for bitshifting an int
    static size_t rotl(size_t value, int shift) {
        return (value << shift) | (value >> (64 - shift));
    };

    static size_t splitmix64(size_t& seed) {
        size_t result = (seed += 0x9E3779B97F4A7C15ULL);
        result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ULL;
        result = (result ^ (result >> 27)) * 0x94D049BB133111EBULL;
        return result ^ (result >> 31);
    };

    /* This is the jump function for the generator. It is equivalent
    to 2^64 calls to next(); it can be used to generate 2^64
    non-overlapping subsequences for parallel computations. */

    void jump() {
        static const std::array<size_t, 2> JUMP = {0xdf900294d8f554a5, 0x170865df4b3201fc};

        size_t state0 = 0;
        size_t state1 = 0;
        for (const auto& jump_val : JUMP) {
            for (int bit_idx = 0; bit_idx < 64; bit_idx++) {
                if ((jump_val & (UINT64_C(1) << bit_idx)) != 0) {
                    state0 ^= state[0];
                    state1 ^= state[1];
                }
                std::ignore = getRandomDouble();
            }
        }

        state[0] = state0;
        state[1] = state1;
    }

    // used to store internal state of RNG
    std::array<size_t, 2> state;
    constexpr static std::string_view OBSERVABLE_NAME{"Potentially observable Xoroshiro RNG"};
    constexpr static std::string_view UNOBSERVABLE_NAME{"Unobservable Xoroshiro RNG"};
};

}  // namespace benchmarks