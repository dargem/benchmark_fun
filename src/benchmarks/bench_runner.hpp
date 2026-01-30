#pragma once

#include <vector>
#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

struct Benchmark {
    
};

// class for running benchmarks
// also a singleton because I like singletons and its tightly linked to the timer singleton
// so there shouldn't be multiple bench runners at a time as the current timing method doesn't allow it
class BenchRunner {
public:
    // Get a reference to the benchrunner
    static BenchRunner& getInstance() {
        static BenchRunner benchRunner; // lazily construct the benchrunner
        return benchRunner;
    }

    BenchRunner(const BenchRunner&) = delete; // no copy constructor
    BenchRunner(BenchRunner&&) = delete; // no move constructor
    void operator=(const BenchRunner&) = delete; // no copy assignment
    void operator=(BenchRunner&&) = delete; // no move assignment

    // Move ownership of the benchable to the bench runner
    void addBenchable(Benchable&& benchable) {
        benchables.push_back(benchable);
    }

    void clearBenchables() {
        benchables.clear();
    }

    void runBenchmarks() {

    }
private:
    BenchRunner() = default;
    std::vector<Benchable> benchables;
};

}