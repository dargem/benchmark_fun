#pragma once

#include <array>
#include <cstdint>
#include <random>

// forward declaration
class NumberGeneratorFactory;

class Xoroshiro128plus
{
public:
    // creates and seeds a generator
    explicit Xoroshiro128plus(uint64_t seed) {
        // splitmix64 modifies the seed so this is deterministic with 2 different modified numbers
        state[0] = splitmix64(seed);
        state[1] = splitmix64(seed);
    }

    /**
     * @brief creates a random uniform double between [0,1)
     * 
     * @return double 
     */
    [[nodiscard]] double getRandomDouble() {
        const uint64_t state0 = state[0];
        uint64_t state1 = state[1];

        // create random number using the internal state
        const uint64_t result = state0 + state1;

        // mix up the internal state again
        state1 ^= state0;
        state[0] = rotl(state0, 24) ^ state1 ^ (state1 << 16); // a, b
        state[1] = rotl(state1, 37); // c

        // return, shifting result into [0,1)
        return (result >> 11) * (1.0 / (1ULL << 53));
    }

private:
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

    // used to store internal state of RNG
    std::array<uint64_t, 2> state;
};