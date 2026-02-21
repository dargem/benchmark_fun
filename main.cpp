#include "src/timer.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include "src/benchmarks/branch_prediction_sorted.hpp"
#include "src/benchmarks/branch_prediction_unsorted.hpp"

#include "src/benchmarks/bench_runner.hpp"
#include <iostream>
#include <memory>

using benchmarks::Xoroshiro128plus;
using benchmarks::BranchPredictionSorted;
using benchmarks::BranchPredictionUnsorted;
using benchmarks::BenchRunner;
using benchmarks::Benchable;

void runRNGBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr size_t ITERATIONS{ 1000000 };
    constexpr size_t NUM_SAMPLES{ 300 };
    constexpr int RNG_SEED{ 50000 };

    {
        // add mersenne twister inbuilt later maybe, half sure this doesn't work
        auto rng = std::make_unique<Xoroshiro128plus<true>>(RNG_SEED);
        benchRunner.addBenchable(std::move(rng));
    }

    benchRunner.runBenchmarks(ITERATIONS, NUM_SAMPLES);
    benchRunner.printResults();

    benchRunner.clearBenchables();
}

void runBranchPredictionBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr size_t ITERATIONS{ 1000 };
    constexpr size_t NUM_SAMPLES{ 100 };
    constexpr int LIST_SIZE{ 10000 }; 
    // this is 10000 iterations over 10000 size list, sampling 100 times of that for each

    {
        auto sortedNumbers = std::make_unique<BranchPredictionSorted>(LIST_SIZE);
        auto unsortedNumbers = std::make_unique<BranchPredictionUnsorted>(LIST_SIZE);
        benchRunner.addBenchable(std::move(sortedNumbers));
        benchRunner.addBenchable(std::move(unsortedNumbers));
    }

    benchRunner.runBenchmarks(ITERATIONS, NUM_SAMPLES);
    benchRunner.printResults();
    benchRunner.clearBenchables();
}

int main() {
    try {
        runRNGBenchmark();
        runBranchPredictionBenchmark();
    } catch(const std::runtime_error e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}