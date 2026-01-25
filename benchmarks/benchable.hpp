#pragma once

#include <cstddef>

namespace benchmarks {

// base class for benchmarks
class Benchable {
public:
    virtual void runBenchmark(size_t iterations) const = 0;
};

};
