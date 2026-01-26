#pragma once

#include <cstddef>
#include "src/benchmarks/benchable.hpp"
#include <vector>
#include <random>

namespace benchmarks {

class BranchPredictionUnsorted : public Benchable {
public:
    BranchPredictionUnsorted(size_t listSize) {
        // get a random seed from the hardware
        std::random_device rd;

        // make a mersenne twister RNG with a distribution for it
        std::mt19937 randomNumberGenerator(rd());
        std::uniform_int_distribution<double> distrib(DISTRIBUTION_MIN, DISTRIBUTION_MAX);
        
        randomNumbers.reserve(listSize);
        for (size_t i{}; i < randomNumbers.size(); ++i) {
            randomNumbers[i] = distrib(randomNumberGenerator);
        }
    }

    // returns a reference to random numbers
    std::vector<double>& getInternalNumbersVector() {
        return randomNumbers;
    }

    void runBenchmark(size_t iterations) override {
        size_t successes{};
        for (size_t i{}; i < iterations; ++i) {
            for (double number : randomNumbers) {
                if (number > SIZE_NEEDED_FOR_SUCCESS) {
                    successes += 1;
                }
            }
        }
        // tell compiler this is volatile this benchmark from being optimized away by the compiler
        asm volatile("" : : "r"(successes) : "memory");
    }

private:
    std::vector<double> randomNumbers;
    static constexpr double DISTRIBUTION_MAX{100};
    static constexpr double DISTRIBUTION_MIN{0};
    static constexpr double SIZE_NEEDED_FOR_SUCCESS{0};
};

};