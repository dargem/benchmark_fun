#include <random>
#include <string>

#include "src/benchmarks/benchable.hpp"
#include "src/utils/xoroshiro_64_star.hpp"

namespace benchmarks {

static inline uint32_t rotl_scalar(const uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }

// bad practice but just for this for shared buffer
alignas(XoroshiroRNG::REGISTER_BYTE_SIZE) static std::array<uint32_t, 100000> arr;

class ScalarXoroshiro64Star {
    uint32_t s[2] = {123456789u, 987654321u};

   public:
    inline uint32_t next_u32() {
        const uint32_t s0 = s[0];
        uint32_t s1 = s[1];

        const uint32_t result = s0 * 0x9E3779BBu;

        s1 ^= s0;
        s[0] = rotl_scalar(s0, 26) ^ s1 ^ (s1 << 9);
        s[1] = rotl_scalar(s1, 13);

        return result;
    }
};

template <size_t N>
    requires(N <= arr.size())
class MersenneTwisterArrayFill : public Benchable {
   public:
    MersenneTwisterArrayFill() : Benchable("Mersenne twister RNG"), randomNumberGenerator(rd()) {};

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            for (size_t j{}; j < N; ++j) {
                arr[j] = unif_dist(randomNumberGenerator);
            }
        }
        asm volatile("" ::"r"(arr.data()) : "memory");
    }

    // reseed it with another hardware generated random number
    void resetBenchmark() override { randomNumberGenerator.seed(rd()); }

   private:
    // get a random seed from the hardware
    std::random_device rd;
    // instantiate a mersenne twister with a distribution fit
    std::mt19937 randomNumberGenerator;
    std::uniform_int_distribution<uint32_t> unif_dist{};
    // std::array<uint32_t, N> arr{};
};

template <size_t N>
    requires(N <= arr.size())
class Xoroshiro64ArrayFill : public Benchable {
   public:
    Xoroshiro64ArrayFill() : Benchable("Xoroshiro64 RNG") { arr.fill(0); }

    void runBenchmark(uint64_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            for (size_t j{}; j < N; ++j) {
                arr[j] = rng.next_u32();
            }
        }
        asm volatile("" ::"r"(arr.data()) : "memory");
    }

    void resetBenchmark() override {}

   private:
    volatile uint32_t sink;
    ScalarXoroshiro64Star rng = ScalarXoroshiro64Star();
    // std::array<uint32_t, N> arr{};
};

template <size_t N>
    requires(N <= arr.size())
class Xoroshiro64BufferedArrayFill : public Benchable {
   public:
    Xoroshiro64BufferedArrayFill() : Benchable("Buffered Xoroshiro64 RNG") { arr.fill(0); }

    void runBenchmark(uint64_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            for (size_t j{}; j < N; ++j) {
                arr[j] = rng.get_uint32();
            }
        }
        asm volatile("" ::"r"(arr.data()) : "memory");
    }

    void resetBenchmark() override {}

   private:
    volatile uint32_t sink;
    SequentialXoroshiroRNG rng = SequentialXoroshiroRNG();
    // std::array<uint32_t, N> arr{};
};

template <size_t N>
    requires(N <= arr.size())
class Xoroshiro64SIMDArrayFill : public Benchable {
   public:
    Xoroshiro64SIMDArrayFill() : Benchable("SIMD Xoroshiro64 RNG") { arr.fill(0); }

    void runBenchmark(uint64_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            rng.fill_aligned_uint32(arr.data(), N);
            asm volatile("" ::"r"(arr.data()) : "memory");
        }
    }

    void resetBenchmark() override {}

   private:
    volatile uint32_t sink;
    XoroshiroRNG rng = XoroshiroRNG();
    // alignas(XoroshiroRNG::REGISTER_BYTE_SIZE) std::array<uint32_t, N> arr{};
};

}  // namespace benchmarks