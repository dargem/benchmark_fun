#pragma once

#include <iostream>
#include <random>

#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

class MersenneTwister : public Benchable {
   public:
    MersenneTwister() : Benchable("Mersenne twister RNG"), randomNumberGenerator(rd()) {};

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            sink = unif_dist(randomNumberGenerator);
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
    volatile double sink{};

    // make a mersenne twister RNG with a distribution for it
};

}  // namespace benchmarks