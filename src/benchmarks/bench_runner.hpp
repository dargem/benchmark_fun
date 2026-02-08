#pragma once

#include <vector>
#include <stdexcept>
#include <math.h>
#include <iostream>
#include <format>
#include <memory>
#include "src/benchmarks/benchable.hpp"
#include "src/timer.hpp"
#include <boost/math/distributions/students_t.hpp>


namespace benchmarks {

constexpr static size_t NUMBER_CLT_TESTS_NEEDED{ 30 };

struct Evaluation {
    Evaluation(std::string_view name, double meanCycles, double standardDeviation, size_t numTests)
        : name{ name }, meanCycles{ meanCycles }, standardDeviation{ standardDeviation }, numTests{ numTests }
    {
        // calculate the confidence interval
        if (numTests < NUMBER_CLT_TESTS_NEEDED) {
            throw std::runtime_error(std::format(
                "More tests needed for central limit theorem; need {} but have {}",
                NUMBER_CLT_TESTS_NEEDED,
                numTests));
        }

        // use central limit theorem to create a confidence interval for the true mean using t distribution
        
        // confidence interval is x_bar ± t * (s / sqrt(n))
        boost::math::students_t dist(numTests - 1);
        const double tScore{ boost::math::quantile(dist, 1.0 - ALPHA / 2.0) };
        const double margin{ tScore * standardDeviation / std::sqrt(static_cast<double>(numTests)) };
        lowerConfidenceInterval = meanCycles - margin;
        upperConfidenceInterval = meanCycles + margin;
    }

    void print() {
        std::cout << "---Summary statistics for " << name << "---"
                  << "\nSample mean cycles per test: " << meanCycles 
                  << "\nConfidence interval: " << lowerConfidenceInterval << "-" << upperConfidenceInterval
                  << "\nSample standard deviation: " << standardDeviation 
                  << "\nTests used: " << numTests << std::endl; 
    }

    const std::string_view name;

    // sampled mean number of cycles it took to do an iteration of the bench
    const double meanCycles;

    // sampled deviation of the averages of each bench
    const double standardDeviation;

    // number of benches this was built from
    const size_t numTests;

    // confidence interval made using CONFIDENCE_ALPHA global variable
    constexpr static double ALPHA{ 0.05 };
    double lowerConfidenceInterval;
    double upperConfidenceInterval;
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
            throw std::runtime_error(std::format(
                "Not enough tests run for central limit theorem; need {} but have {}",
                NUMBER_CLT_TESTS_NEEDED,
                numTests));
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
    std::vector<size_t> clockTimes;
};

class BenchmarkCollection {
public:
    void addTime(std::string_view name, size_t timeTaken) {
        for (Benchmark& bench : benchmarks) {
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
            evaluations.back().print();
        }
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
    void addBenchable(std::unique_ptr<Benchable> newBenchable) {
        for (const auto& benchable : benchables) {
            if (typeid(benchable) == typeid(newBenchable)) {
                throw std::runtime_error("Shouldn't have two of the same benchables");
            }
        }
        benchables.push_back(std::move(newBenchable));
    }

    void clearBenchables() {
        benchables.clear();
    }

    void runBenchmarks(size_t iterations, size_t numSamples) {
        for (size_t i{}; i < numSamples; ++i) {
            // Take numSamples samples for each bench
            for (const auto& benchable : benchables) {
                // run the benchmark
                timer.startTimer();
                benchable->runBenchmark(iterations);
                size_t cyclesTaken = timer.endTimer();
                benchable->resetBenchmark();

                // add results into the benchmark collection
                benchmarkCollection.addTime(benchable->getName(), cyclesTaken);
            }
        }
    }

    void printResults() {
        benchmarkCollection.print();
    }

private:

    BenchRunner() = default;
    Timer& timer = Timer::getInstance();
    std::vector<std::unique_ptr<Benchable>> benchables;
    BenchmarkCollection benchmarkCollection;
};

}