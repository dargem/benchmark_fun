#include <iostream>
#include <memory>

#include "src/benchmarks/SOA_AOS/structures.hpp"
#include "src/benchmarks/bench_runner.hpp"
#include "src/benchmarks/branch_prediction/branch_prediction_sorted.hpp"
#include "src/benchmarks/branch_prediction/branch_prediction_unsorted.hpp"
#include "src/benchmarks/rng/mersenne_twister.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include "src/benchmarks/vector_access/vectors.hpp"
#include "src/timer.hpp"

using benchmarks::AOS;
using benchmarks::Benchable;
using benchmarks::BenchRunner;
using benchmarks::BranchPredictionSorted;
using benchmarks::BranchPredictionUnsorted;
using benchmarks::MersenneTwister;
using benchmarks::SOA;
using benchmarks::VectorAccess;
using benchmarks::Xoroshiro128plus;

void runRNGBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr size_t ITERATIONS{1000000};
    constexpr size_t NUM_SAMPLES{300};
    constexpr int RNG_SEED{50000};

    {
        auto customRNG = std::make_unique<Xoroshiro128plus>(RNG_SEED);
        auto mersenneRNG = std::make_unique<MersenneTwister>();
        benchRunner.addBenchable(std::move(customRNG));
        benchRunner.addBenchable(std::move(mersenneRNG));
    }

    benchRunner.runBenchmarks(ITERATIONS, NUM_SAMPLES);
    benchRunner.printResults();

    benchRunner.clearBenchables();
}

void runBranchPredictionBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr size_t ITERATIONS{120};
    constexpr size_t NUM_SAMPLES{300};
    constexpr int LIST_SIZE{10000};
    // this is 100 iterations over 10000 size list, sampling 300 times of that for each

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

void runVectorRandomAccessBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr size_t numElements{10000};
    constexpr size_t numIndicies{1000};

    {
        auto boolAccessBench = std::make_unique<VectorAccess<bool>>(numElements, numIndicies);
        auto charAccessBench = std::make_unique<VectorAccess<char>>(numElements, numIndicies);
        auto uint8_tAccessBench = std::make_unique<VectorAccess<uint8_t>>(numElements, numIndicies);

        benchRunner.addBenchable(std::move(boolAccessBench));
        benchRunner.addBenchable(std::move(charAccessBench));
        benchRunner.addBenchable(std::move(uint8_tAccessBench));
    }

    constexpr size_t ITERATIONS{1};
    constexpr size_t NUM_SAMPLES{1000};
    benchRunner.runBenchmarks(ITERATIONS, NUM_SAMPLES);
    benchRunner.printResults();
    benchRunner.clearBenchables();
}

void testSOA_AOS_Iteration() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr size_t NUM_ENTITIES{50000};
    constexpr size_t ITERATIONS{1};
    constexpr size_t SAMPLES{500};

    {
        auto AOS_struct8bit = std::make_unique<AOS<uint8_t>>(NUM_ENTITIES);
        auto SOA_struct8bit = std::make_unique<SOA<uint8_t>>(NUM_ENTITIES);
        benchRunner.addBenchable(std::move(AOS_struct8bit));
        benchRunner.addBenchable(std::move(SOA_struct8bit));
        benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
        benchRunner.printResults();
        benchRunner.clearBenchables();
    }

    {
        auto AOS_structFloat = std::make_unique<AOS<float>>(NUM_ENTITIES);
        auto SOA_structFloat = std::make_unique<SOA<float>>(NUM_ENTITIES);
        benchRunner.addBenchable(std::move(AOS_structFloat));
        benchRunner.addBenchable(std::move(SOA_structFloat));
        benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
        benchRunner.printResults();
        benchRunner.clearBenchables();
    }

    {
        auto AOS_structDouble = std::make_unique<AOS<double>>(NUM_ENTITIES);
        auto SOA_structDouble = std::make_unique<SOA<double>>(NUM_ENTITIES);
        benchRunner.addBenchable(std::move(AOS_structDouble));
        benchRunner.addBenchable(std::move(SOA_structDouble));
        benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
        benchRunner.printResults();
        benchRunner.clearBenchables();
    }

    {
        auto AOS_structLongDouble = std::make_unique<AOS<long double>>(NUM_ENTITIES);
        auto SOA_structLongDouble = std::make_unique<SOA<long double>>(NUM_ENTITIES);
        benchRunner.addBenchable(std::move(AOS_structLongDouble));
        benchRunner.addBenchable(std::move(SOA_structLongDouble));
        benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
        benchRunner.printResults();
        benchRunner.clearBenchables();
    }
}

int main() {
    try {
        runRNGBenchmark();
        runBranchPredictionBenchmark();
        runVectorRandomAccessBenchmark();
        testSOA_AOS_Iteration();
    } catch (const std::runtime_error e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}