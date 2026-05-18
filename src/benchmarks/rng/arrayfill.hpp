#include <random>
#include <string>

#include "src/benchmarks/benchable.hpp"
#include "src/utils/xoroshiro_64_star.hpp"

namespace benchmarks {

template <size_t N>
class MersenneTwisterArrayFill : public Benchable {
   public:
    MersenneTwisterArrayFill() : Benchable("Mersenne twister RNG"), randomNumberGenerator(rd()) {};

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            for (uint32_t& e : arr) {
                e = unif_dist(randomNumberGenerator);
            }
            asm volatile("" ::"r"(arr.data()) : "memory");
        }
    }

    // reseed it with another hardware generated random number
    void resetBenchmark() override { randomNumberGenerator.seed(rd()); }

   private:
    // get a random seed from the hardware
    std::random_device rd;
    // instantiate a mersenne twister with a distribution fit
    std::mt19937 randomNumberGenerator;
    std::uniform_int_distribution<uint32_t> unif_dist{};
    std::array<uint32_t, N> arr{};
};

template <size_t N>
class Xoroshiro64BufferedArrayFill : public Benchable {
   public:
    Xoroshiro64BufferedArrayFill() : Benchable("Buffered Xoroshiro64 RNG") { arr.fill(0); }

    void runBenchmark(uint64_t iterations) override {
        for (uint64_t i{}; i < iterations; ++i) {
            for (uint32_t& val : arr) {
                val = rng.get_uint32();
            }
            asm volatile("" ::"r"(arr.data()) : "memory");
        }
    }

    void resetBenchmark() override {}

   private:
    volatile uint32_t sink;
    SequentialXoroshiroRNG rng = SequentialXoroshiroRNG();
    std::array<uint32_t, N> arr{};
};

template <size_t N>
class Xoroshiro64SIMDArrayFill : public Benchable {
   public:
    Xoroshiro64SIMDArrayFill() : Benchable("SIMD Xoroshiro64 RNG") { arr.fill(0); }

    void runBenchmark(uint64_t iterations) override {
        for (uint64_t i{}; i < iterations; ++i) {
            rng.fill_aligned_uint32(arr.data(), N);
            asm volatile("" ::"r"(arr.data()) : "memory");
        }
    }

    void resetBenchmark() override {}

   private:
    volatile uint32_t sink;
    XoroshiroRNG rng = XoroshiroRNG();
    alignas(XoroshiroRNG::REGISTER_BYTE_SIZE) std::array<uint32_t, N> arr{};
};

}  // namespace benchmarks