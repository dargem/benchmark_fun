#pragma once

#include <vector>
#include <stdexcept>
#include <math.h>
#include "src/benchmarks/benchable.hpp"
namespace benchmarks {

struct Evaluation {
    Evaluation(double meanCycles, double standardDeviation, double numTests)
        : meanCycles{ meanCycles }, standardDeviation{ standardDeviation }, numTests{ numTests }
    {}

    // sampled mean number of cycles it took to do an iteration of the bench
    const double meanCycles;

    // sampled deviation of the averages of each bench
    const double standardDeviation;

    // number of benches this was built from
    const double numTests;
};

struct Benchmark {
    Benchmark(std::string_view name)
        : name{ name }
    {}

    Evaluation createEvaluation() {

        size_t numTests{ clockTimes.size() };

        if (numTests < 30) {
            std::runtime_error("Not enough tests run for central limit theorem");
        }

        double totalCycles{};
        for (size_t clockTime : clockTimes) {
            totalCycles += clockTime;
        }
        double sampleMean{ totalCycles / numTests };

        double variance{};
        for (size_t clockTime : clockTimes) {
            variance += std::pow(clockTime - sampleMean, 2);
        }

        double standardDeviation{ std::pow(variance/(numTests - 1), 0.5) };
        return Evaluation(sampleMean, standardDeviation, numTests);
    }

    const std::string_view name;
    std::vector<const size_t> clockTimes;
};

class BenchmarkCollection {
public:
    // print the results of the benchmark out
    void print() {

    }

    

private:
    std::vector<Benchmark> benchmarks;
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
    void addBenchable(Benchable&& newBenchable) {
        for (Benchable& benchable : benchables) {
            if (typeid(benchable) == typeid(newBenchable)) {
                throw std::runtime_error("Shouldn't have two of the same benchables");
            }
        }
        benchables.push_back(newBenchable);
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