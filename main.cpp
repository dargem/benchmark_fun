#include <iostream>
#include <stdfloat>

#include "src/benchmarks/SOA_AOS/structures.hpp"
#include "src/benchmarks/SSO/small_string_optimisation.hpp"
#include "src/benchmarks/alignment/alignment.hpp"
#include "src/benchmarks/allocation/allocation.hpp"
#include "src/benchmarks/attributes/likely_attributes.hpp"
#include "src/benchmarks/bench_runner.hpp"
#include "src/benchmarks/binary_search/binary_search.hpp"
#include "src/benchmarks/branch_prediction/branch_prediction_sorted.hpp"
#include "src/benchmarks/branch_prediction/branch_prediction_unsorted.hpp"
#include "src/benchmarks/execution_policies/policies.hpp"
#include "src/benchmarks/ring_buffers/cachy.hpp"
#include "src/benchmarks/ring_buffers/classic.hpp"
#include "src/benchmarks/ring_buffers/condensed_cachy.hpp"
#include "src/benchmarks/ring_buffers/ring_buffer_tester.hpp"
#include "src/benchmarks/rng/arrayfill.hpp"
#include "src/benchmarks/rng/mersenne_twister.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include "src/benchmarks/vector_access/vectors.hpp"
#include "src/benchmarks/weird_vector/reserve_vector.hpp"
#include "src/stats/anovas.hpp"

using benchmarks::AllocationBench;
using benchmarks::Allocator;
using benchmarks::AOS;
using benchmarks::ArrayWrite;
using benchmarks::Attribute;
using benchmarks::AttributeOptimisation;
using benchmarks::BranchPredictionSorted;
using benchmarks::BranchPredictionUnsorted;
using benchmarks::BufferTester;
using benchmarks::ExecutionPolicies;
using benchmarks::MersenneTwister;
using benchmarks::MersenneTwisterArrayFill;
using benchmarks::Policy;
using benchmarks::ReservationSize;
using benchmarks::SOA;
using benchmarks::StandardBinarySearch;
using benchmarks::StringRunner;
using benchmarks::VectorAccess;
using benchmarks::VectorWrapper;
using benchmarks::Xoroshiro128plus;
using benchmarks::Xoroshiro64ArrayFill;
using benchmarks::Xoroshiro64BufferedArrayFill;
using benchmarks::Xoroshiro64SIMDArrayFill;
using stats::sameGroupMeans;

void runRNGBenchmark() {
    constexpr size_t ITERATIONS{1000000};
    constexpr size_t NUM_SAMPLES{300};
    constexpr int RNG_SEED{50000};

    auto customRNG = Xoroshiro128plus(RNG_SEED);
    auto mersenneRNG = MersenneTwister();
    benchmarks::executeBench(ITERATIONS, NUM_SAMPLES, customRNG, mersenneRNG);
}

void runBranchPredictionBenchmark() {
    constexpr size_t ITERATIONS{120};
    constexpr size_t NUM_SAMPLES{300};
    constexpr int LIST_SIZE{10000};
    // this is 100 iterations over 10000 size list, sampling 300 times of that for each

    auto sortedNumbers = BranchPredictionSorted(LIST_SIZE);
    auto unsortedNumbers = BranchPredictionUnsorted(LIST_SIZE);
    benchmarks::executeBench(ITERATIONS, NUM_SAMPLES, sortedNumbers, unsortedNumbers);
}

void runVectorRandomAccessBenchmark() {
    constexpr size_t numElements{10000};
    constexpr size_t numIndicies{1000};
    constexpr size_t ITERATIONS{1};
    constexpr size_t NUM_SAMPLES{1000};

    auto boolAccessBench = VectorAccess<bool>(numElements, numIndicies);
    auto charAccessBench = VectorAccess<char>(numElements, numIndicies);
    auto uint8_tAccessBench = VectorAccess<uint8_t>(numElements, numIndicies);

    benchmarks::executeBench(ITERATIONS, NUM_SAMPLES, boolAccessBench, charAccessBench,
                             uint8_tAccessBench);
}

void testSOA_AOS_Iteration() {
    constexpr size_t NUM_ENTITIES{100000};
    constexpr size_t ITERATIONS{1};
    constexpr size_t SAMPLES{1000};

    {
        auto AOS_struct8bit = AOS<uint8_t>(NUM_ENTITIES);
        auto SOA_struct8bit = SOA<uint8_t>(NUM_ENTITIES);
        benchmarks::executeBench(ITERATIONS, SAMPLES, AOS_struct8bit, SOA_struct8bit);
    }

    {
        auto AOS_structFloat = AOS<float>(NUM_ENTITIES);
        auto SOA_structFloat = SOA<float>(NUM_ENTITIES);
        benchmarks::executeBench(ITERATIONS, SAMPLES, AOS_structFloat, SOA_structFloat);
    }

    {
        auto AOS_structDouble = AOS<double>(NUM_ENTITIES);
        auto SOA_structDouble = SOA<double>(NUM_ENTITIES);
        benchmarks::executeBench(ITERATIONS, SAMPLES, AOS_structDouble, SOA_structDouble);
    }

    {
        auto AOS_structLongDouble = AOS<long double>(NUM_ENTITIES);
        auto SOA_structLongDouble = SOA<long double>(NUM_ENTITIES);
        benchmarks::executeBench(ITERATIONS, SAMPLES, AOS_structLongDouble, SOA_structLongDouble);
    }

    {
        auto AOS_halfFloat = AOS<std::float16_t>(NUM_ENTITIES);
        auto SOA_halfFloat = SOA<std::float16_t>(NUM_ENTITIES);
        benchmarks::executeBench(ITERATIONS, SAMPLES, AOS_halfFloat, SOA_halfFloat);
    }
}

void runAttributeBenchmark() {
    constexpr static size_t LIST_SIZE{50000};

    constexpr static size_t ITERATIONS{5};
    constexpr static size_t SAMPLES{5000};

    auto likely = AttributeOptimisation<Attribute::LIKELY>(LIST_SIZE);
    auto unlikely = AttributeOptimisation<Attribute::UNLIKELY>(LIST_SIZE);
    auto defaultBench = AttributeOptimisation<Attribute::DEFAULT>(LIST_SIZE);
    auto branchless = AttributeOptimisation<Attribute::BRANCHLESS>(LIST_SIZE);

    benchmarks::executeBench(ITERATIONS, SAMPLES, likely, unlikely, defaultBench, branchless);
}

void runExecutionPolicyBenchmark() {
    constexpr static size_t NUM_COORDINATES{50000};

    constexpr static size_t ITERATIONS{10};
    constexpr static size_t SAMPLES{50};

    auto indexedLoop = ExecutionPolicies<Policy::INDEXED_LOOP>(NUM_COORDINATES);
    auto sequential = ExecutionPolicies<Policy::SEQUENCED>(NUM_COORDINATES);
    auto unsequenced = ExecutionPolicies<Policy::UNSEQUENCED>(NUM_COORDINATES);
    auto parallel = ExecutionPolicies<Policy::PARALLEL>(NUM_COORDINATES);
    auto par_unseq = ExecutionPolicies<Policy::PARALLEL_UNSEQUENCED>(NUM_COORDINATES);

    benchmarks::executeBench(ITERATIONS, SAMPLES, indexedLoop, sequential, unsequenced, parallel,
                             par_unseq);
}

void runReservedVectorBenchmark() {
    // given each element inside is 8 bytes, 3.2 GB
    constexpr static size_t ELEMENTS{5000000};
    constexpr static size_t SAMPLES{30};

    auto noReserveVector = VectorWrapper<ReservationSize::ZERO_BYTES>();
    // auto bigReserveVector = VectorWrapper<ReservationSize::FIFTEEN_GIGABYTE>();

    benchmarks::executeBench(ELEMENTS, SAMPLES, noReserveVector);
}

void runStringOptimsationBenchmark() {
    auto string15 = StringRunner<15>();
    auto string16 = StringRunner<16>();
    auto string17 = StringRunner<17>();
    auto string18 = StringRunner<18>();

    constexpr static size_t ITERATIONS{10000};
    constexpr static size_t SAMPLES{300};

    benchmarks::executeBench(ITERATIONS, SAMPLES, string15, string16, string17, string18);
}

void runArrayWriteBenchmark() {
    constexpr static size_t ITERATIONS{500000};
    constexpr static size_t SAMPLES{500};

    // We are writing 8 bytes at this index
    auto access0 = ArrayWrite<0>();    // aligned
    auto access7 = ArrayWrite<7>();    // unaligned
    auto access8 = ArrayWrite<8>();    // aligned
    auto access13 = ArrayWrite<13>();  // unaligned
    auto access16 = ArrayWrite<16>();  // aligned
    auto access32 = ArrayWrite<32>();  // aligned
    auto access55 = ArrayWrite<55>();  // unaligned
    auto access56 = ArrayWrite<56>();  // aligned
    auto access57 = ArrayWrite<57>();  // unaligned and crossing page boundary
    auto access58 = ArrayWrite<58>();  // unaligned and crossing page boundary
    auto access59 = ArrayWrite<59>();  // unaligned and crossing page boundary
    auto access63 = ArrayWrite<63>();  // unaligned and crossing page boundary

    benchmarks::executeBench(ITERATIONS, SAMPLES, access0, access7, access8, access13, access16,
                             access32, access55, access56, access57, access58, access59, access63);
}

void arrayFill() {
    constexpr static size_t ITERATIONS{100};
    constexpr static size_t ARRAY_SIZE{100000};
    constexpr static size_t SAMPLES{50};

    auto mersenne = MersenneTwisterArrayFill<ARRAY_SIZE>();
    auto scalarXORO = Xoroshiro64ArrayFill<ARRAY_SIZE>();
    auto bufferedXORO = Xoroshiro64BufferedArrayFill<ARRAY_SIZE>();
    auto simdXORO = Xoroshiro64SIMDArrayFill<ARRAY_SIZE>();

    benchmarks::executeBench(ITERATIONS, SAMPLES, mersenne, scalarXORO, bufferedXORO, simdXORO);
}

void ringBufferImplementations() {
    constexpr static size_t BUFFER_CAPACITY{1000000};
    constexpr static size_t ITERATIONS{100000000};
    constexpr static size_t SAMPLES{100};

    auto standard = BufferTester<RingBuffer>(BUFFER_CAPACITY);
    auto cachy = BufferTester<CachingRingBuffer>(BUFFER_CAPACITY);
    auto mine = BufferTester<CachingRingBufferCompressed>(BUFFER_CAPACITY);

    benchmarks::executeBench(ITERATIONS, SAMPLES, standard, cachy, mine);
};

// void binarySearchLayouts() {
//     constexpr static size_t ITERATIONS{1};
//     constexpr static size_t NUM_ELEMENTS{100000};
//     constexpr static size_t NUM_SEARCHED{1000};
//     constexpr static size_t SAMPLES{100};

//     auto standard = StandardBinarySearch<NUM_ELEMENTS, NUM_SEARCHED>();

//     benchmarks::executeBench(ITERATIONS, SAMPLES, standard);
// }

void allocatorBench() {
    constexpr static size_t ITERATIONS{500000};
    constexpr static size_t SAMPLES{100};

    auto new_alloc = AllocationBench<Allocator::NEW>();
    auto arena_alloc = AllocationBench<Allocator::ARENA>();

    benchmarks::executeBench(ITERATIONS, SAMPLES, new_alloc, arena_alloc);
}

void allocationAndDeletionBench() {
    constexpr static size_t ITERATIONS{500000};
    constexpr static size_t SAMPLES{100};

    auto new_alloc = AllocationBench<Allocator::NEW, true>();
    auto arena_alloc = AllocationBench<Allocator::ARENA, true>();

    benchmarks::executeBench(ITERATIONS, SAMPLES, new_alloc, arena_alloc);
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
        // runStringOptimsationBenchmark();
        // runArrayWriteBenchmark();
        // arrayFill();
        // binarySearchLayouts();
        // ringBufferImplementations();
        allocatorBench();
        allocationAndDeletionBench();
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Caught unknown exception type" << std::endl;
    }

    return 0;
}