#pragma once

#include <cstddef>
#include <string_view>
#include "src/benchmarks/bench_types.hpp"

namespace benchmarks {

// base class for benchmarks
class Benchable {
public:
    Benchable(BenchType benchType, std::string_view name) 
        : benchType{ benchType }, name{ name }
    {}

    // call when reusing a benchmark, assume benchables will need some setup
    virtual void resetBenchmark() = 0;
    // call to run the benchmark for the number of iterations
    virtual void runBenchmark(size_t iterations) = 0;

    bool operator==(const Benchable& other) {
        return name == other.name;
    }

    bool isCompatableType(const Benchable& other) {
        return benchType == other.benchType;
    }
private:
    const BenchType benchType;
    const std::string_view name;
};

}