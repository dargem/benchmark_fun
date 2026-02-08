#pragma once

#include <vector>
#include <stdexcept>
#include <math.h>
#include <iostream>
#include <format>
#include "src/benchmarks/benchable.hpp"
namespace benchmarks {

constexpr static size_t NUMBER_CLT_TESTS_NEEDED{ 30 };

struct Evaluation {
    Evaluation(std::string_view name, double meanCycles, double standardDeviation, size_t numTests)
        : name{ name }, meanCycles{ meanCycles }, standardDeviation{ standardDeviation }, numTests{ numTests }
    {
        // calculate the confidence interval
        if (numTests < NUMBER_CLT_TESTS_NEEDED) {
            throw new std::runtime_error(std::format("More tests need for central limit theorem, needed {} has {}", NUMBER_CLT_TESTS_NEEDED, numTests));
        }
    }

    const std::string_view name;

    // sampled mean number of cycles it took to do an iteration of the bench
    const double meanCycles;

    // sampled deviation of the averages of each bench
    const double standardDeviation;

    // number of benches this was built from
    const size_t numTests;

    // confidence interval made using CONFIDENCE_ALPHA global variable
    constexpr static double CONFIDENCE_ALPHA{ 0.05 };
    const double lowerConfidenceInterval;
    const double upperConfidenceInterval;
};

struct Benchmark {
    Benchmark(std::string_view name, size_t clockTime)
        : name{ name }
    {
        clockTimes.push_back(clockTime);
    }

    Evaluation createEvaluation() {

        size_t numTests{ clockTimes.size() };

        if (numTests < NUMBER_CLT_TESTS_NEEDED) {
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
        return Evaluation(name, sampleMean, standardDeviation, numTests);
    }

    
    const std::string_view name;
    std::vector<const size_t> clockTimes;
};

class BenchmarkCollection {
public:
    void addTime(std::string_view name, size_t timeTaken) {
        for (Benchmark bench : benchmarks) {
            if (bench.name == name) {
                bench.clockTimes.push_back(timeTaken);
                return;
            }
        }

        // if its not in
        benchmarks.push_back({name, timeTaken});
    }    

    // print the results of the benchmark out
    void print() {
        std::vector<Evaluation> evaluations;
        evaluations.reserve(benchmarks.size());

        for (Benchmark bench : benchmarks) {
            evaluations.push_back(bench.createEvaluation());
        }
    }

private:
    std::vector<Benchmark> benchmarks;

    void printEvaluation(Evaluation evaluation) {

    }
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