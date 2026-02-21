#pragma once

#include <cstddef>
#include "src/benchmarks/benchable.hpp"
#include "src/benchmarks/bench_types.hpp"
#include <vector>
#include <random>
#include <string_view>

namespace benchmarks {

class BranchPredictionUnsorted : public Benchable {
public:

    BranchPredictionUnsorted(size_t listSize, std::string_view name)
        : Benchable(BenchType::BRANCH_PREDICTION, name),
        listSize{ listSize }
    {
        resetBenchmark();
    }
        
    BranchPredictionUnsorted(size_t listSize) 
        : Benchable(BenchType::BRANCH_PREDICTION, NAME), 
        listSize{ listSize }
    {
        resetBenchmark();
    }

    // returns a reference to random numbers
    std::vector<int>& getInternalNumbersVector() {
        return randomNumbers;
    }

    void resetBenchmark() override {
        // get a random seed from the hardware
        std::random_device rd;

        // make a mersenne twister RNG with a distribution for it
        std::mt19937 randomNumberGenerator(rd());
        std::uniform_int_distribution<int> distrib(DISTRIBUTION_MIN, DISTRIBUTION_MAX);
        
        randomNumbers.resize(listSize);
        for (size_t i{}; i < randomNumbers.size(); ++i) {
            randomNumbers[i] = distrib(randomNumberGenerator);
        }
    }

    void runBenchmark(size_t iterations) override {
        size_t successes{};
        for (size_t i{}; i < iterations; ++i) {
            for (int number : randomNumbers) {
                if (number > SIZE_NEEDED_FOR_SUCCESS) {
                    successes += 1;
                    consume(successes);
                }
            }
        }
        // tell compiler this is volatile this benchmark from being optimized away by the compiler
        asm volatile("" : : "r"(successes) : "memory");
    }

private:
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((noinline))
#endif
    static void consume(size_t& value) {
        asm volatile("" : "+r"(value) : : "memory");
    }

    static constexpr std::string_view NAME{"Branch Prediction Unsorted Version"};
    std::vector<int> randomNumbers;
    const size_t listSize;
    static constexpr int DISTRIBUTION_MAX{100000};
    static constexpr int DISTRIBUTION_MIN{0};
    static constexpr int SIZE_NEEDED_FOR_SUCCESS{(DISTRIBUTION_MAX + DISTRIBUTION_MIN)/2};
};

}