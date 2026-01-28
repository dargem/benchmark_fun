#pragma once

#include <cstddef>
#include <string_view>

namespace benchmarks {

// base class for benchmarks
class Benchable {
public:
    // call when reusing a benchmark, assume benchables will need some setup
    virtual void resetBenchmark() = 0;
    // call to run the benchmark for the number of iterations
    virtual void runBenchmark(size_t iterations) = 0;
    // get the name of this benchmark
    virtual std::string_view getName() = 0;
};

}