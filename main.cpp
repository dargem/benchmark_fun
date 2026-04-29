#include <iostream>
#include <memory>
#include <stdfloat>

#include "src/benchmarks/SOA_AOS/structures.hpp"
#include "src/benchmarks/SSO/small_string_optimisation.hpp"
#include "src/benchmarks/attributes/likely_attributes.hpp"
#include "src/benchmarks/bench_runner.hpp"
#include "src/benchmarks/branch_prediction/branch_prediction_sorted.hpp"
#include "src/benchmarks/branch_prediction/branch_prediction_unsorted.hpp"
#include "src/benchmarks/execution_policies/policies.hpp"
#include "src/benchmarks/rng/mersenne_twister.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include "src/benchmarks/vector_access/vectors.hpp"
#include "src/benchmarks/weird_vector/reserve_vector.hpp"
#include "src/stats/anovas.hpp"
#include "src/stats/student_T_tests.hpp"
#include "src/timer.hpp"

using benchmarks::AOS;
using benchmarks::Attribute;
using benchmarks::AttributeOptimisation;
using benchmarks::Benchable;
using benchmarks::BenchRunner;
using benchmarks::BranchPredictionSorted;
using benchmarks::BranchPredictionUnsorted;
using benchmarks::ExecutionPolicies;
using benchmarks::MersenneTwister;
using benchmarks::Policy;
using benchmarks::ReservationSize;
using benchmarks::SOA;
using benchmarks::StringRunner;
using benchmarks::VectorAccess;
using benchmarks::VectorWrapper;
using benchmarks::Xoroshiro128plus;
using stats::sameGroupMeans;

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

    constexpr size_t NUM_ENTITIES{100000};
    constexpr size_t ITERATIONS{1};
    constexpr size_t SAMPLES{1000};

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

    {
        auto AOS_halfFloat = std::make_unique<AOS<std::float16_t>>(NUM_ENTITIES);
        auto SOA_halfFloat = std::make_unique<SOA<std::float16_t>>(NUM_ENTITIES);
        benchRunner.addBenchable(std::move(AOS_halfFloat));
        benchRunner.addBenchable(std::move(SOA_halfFloat));
        benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
        benchRunner.printResults();
        benchRunner.clearBenchables();
    }
}

void runAttributeBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr static size_t LIST_SIZE{50000};

    {
        auto likely = std::make_unique<AttributeOptimisation<Attribute::LIKELY>>(LIST_SIZE);
        auto unlikely = std::make_unique<AttributeOptimisation<Attribute::UNLIKELY>>(LIST_SIZE);
        auto defaultBench = std::make_unique<AttributeOptimisation<Attribute::DEFAULT>>(LIST_SIZE);

        benchRunner.addBenchable(std::move(likely));
        benchRunner.addBenchable(std::move(unlikely));
        benchRunner.addBenchable(std::move(defaultBench));
    }

    constexpr static size_t ITERATIONS{5};
    constexpr static size_t SAMPLES{5000};

    benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
    benchRunner.printResults();
    benchRunner.clearBenchables();
}

void runExecutionPolicyBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    constexpr static size_t NUM_COORDINATES{50000};

    {
        // clang-format off
        auto indexedLoop = std::make_unique<ExecutionPolicies<Policy::INDEXED_LOOP>>(NUM_COORDINATES);
        auto sequential = std::make_unique<ExecutionPolicies<Policy::SEQUENCED>>(NUM_COORDINATES);
        auto unsequenced = std::make_unique<ExecutionPolicies<Policy::UNSEQUENCED>>(NUM_COORDINATES);
        auto parallel = std::make_unique<ExecutionPolicies<Policy::PARALLEL>>(NUM_COORDINATES);
        auto par_unseq = std::make_unique<ExecutionPolicies<Policy::PARALLEL_UNSEQUENCED>>(NUM_COORDINATES);
        // clang-format on
        benchRunner.addBenchable(std::move(indexedLoop));
        benchRunner.addBenchable(std::move(sequential));
        benchRunner.addBenchable(std::move(unsequenced));
        benchRunner.addBenchable(std::move(parallel));
        benchRunner.addBenchable(std::move(par_unseq));
    }

    constexpr static size_t ITERATIONS{10};
    constexpr static size_t SAMPLES{50};

    benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
    benchRunner.printResults();
    benchRunner.clearBenchables();
}

void runReservedVectorBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    {
        auto noReserveVector = std::make_unique<VectorWrapper<ReservationSize::ZERO_BYTES>>();
        auto bigReserveVector =
            std::make_unique<VectorWrapper<ReservationSize::FIFTEEN_GIGABYTE>>();

        benchRunner.addBenchable(std::move(noReserveVector));
        // benchRunner.addBenchable(std::move(bigReserveVector));
    }

    // given each element inside is 8 bytes, 3.2 GB
    constexpr static size_t ELEMENTS{5000000};
    constexpr static size_t SAMPLES{300000};

    benchRunner.runBenchmarks(ELEMENTS, SAMPLES);
    benchRunner.printResults();
    benchRunner.clearBenchables();
}

void runStringOptimsationBenchmark() {
    BenchRunner& benchRunner = BenchRunner::getInstance();
    benchRunner.clearBenchables();

    {
        auto string15 = std::make_unique<StringRunner<15>>();
        auto string16 = std::make_unique<StringRunner<16>>();
        auto string17 = std::make_unique<StringRunner<17>>();
        auto string18 = std::make_unique<StringRunner<18>>();

        benchRunner.addBenchable(std::move(string15));
        benchRunner.addBenchable(std::move(string16));
        benchRunner.addBenchable(std::move(string17));
        benchRunner.addBenchable(std::move(string18));
    }

    constexpr static size_t ITERATIONS{10000};
    constexpr static size_t SAMPLES{300};

    benchRunner.runBenchmarks(ITERATIONS, SAMPLES);
    benchRunner.printResults();
    benchRunner.clearBenchables();
}

int main() {
    try {
        // runRNGBenchmark();
        // runBranchPredictionBenchmark();
        // runVectorRandomAccessBenchmark();
        // testSOA_AOS_Iteration();
        // runAttributeBenchmark();
        // runExecutionPolicyBenchmark();
        // runReservedVectorBenchmark();
        runStringOptimsationBenchmark();
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Caught unknown exception type" << std::endl;
    }

    return 0;
}