#pragma once

#include <math.h>

#include <boost/math/distributions/fisher_f.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <concepts>
#include <format>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "src/benchmarks/benchable.hpp"
#include "src/timer.hpp"

namespace benchmarks {

constexpr static size_t NUMBER_CLT_TESTS_NEEDED{30};

struct Evaluation {
    Evaluation(std::string_view name, const std::vector<size_t> times, double meanCycles,
               double standardDeviation, size_t numTests) :
            name{name},
            observationValues{times},
            meanCycles{meanCycles},
            standardDeviation{standardDeviation},
            numTests{numTests} {
        // calculate the confidence interval
        if (numTests < NUMBER_CLT_TESTS_NEEDED) {
            throw std::runtime_error(
                std::format("More tests needed for central limit theorem; need {} but have {}",
                            NUMBER_CLT_TESTS_NEEDED, numTests));
        }

        // use central limit theorem to create a confidence interval for the true mean using t
        // distribution

        // confidence interval is x_bar ± t * (s / sqrt(n))
        boost::math::students_t dist(numTests - 1);
        const double tScore{boost::math::quantile(dist, 1.0 - ALPHA / 2.0)};
        const double margin{tScore * standardDeviation / std::sqrt(static_cast<double>(numTests))};
        lowerConfidenceInterval = meanCycles - margin;
        upperConfidenceInterval = meanCycles + margin;
    }

    void print() {
        std::cout << "---Summary statistics for " << name << "---"
                  << "\nSample mean cycles per test: " << meanCycles
                  << "\nConfidence interval: " << lowerConfidenceInterval << "-"
                  << upperConfidenceInterval << "\nSample standard deviation: " << standardDeviation
                  << "\nTests used: " << numTests << std::endl;
    }

    const std::string_view name;

    // kinda hacky but some stuff like levene's need access to test values not just summary stats
    const std::vector<size_t> observationValues;

    // sampled mean number of cycles it took to do an iteration of the bench
    const double meanCycles;

    // sampled deviation of the averages of each bench
    const double standardDeviation;

    // number of benches this was built from
    const size_t numTests;

    // confidence interval made using CONFIDENCE_ALPHA global variable
    constexpr static double ALPHA{0.05};
    double lowerConfidenceInterval;
    double upperConfidenceInterval;
};

struct Benchmark {
    Benchmark(std::string_view name, size_t clockTime) : name{name} {
        clockTimes.push_back(clockTime);
    }

    Evaluation createEvaluation() {
        size_t numTests{clockTimes.size()};

        if (numTests < NUMBER_CLT_TESTS_NEEDED) {
            throw std::runtime_error(
                std::format("Not enough tests run for central limit theorem; need {} but have {}",
                            NUMBER_CLT_TESTS_NEEDED, numTests));
        }

        double totalCycles{};
        for (size_t clockTime : clockTimes) {
            totalCycles += clockTime;
        }
        double sampleMean{totalCycles / numTests};

        double variance{};
        for (size_t clockTime : clockTimes) {
            variance += std::pow(clockTime - sampleMean, 2);
        }

        double standardDeviation{std::pow(variance / (numTests - 1), 0.5)};
        return Evaluation(name, clockTimes, sampleMean, standardDeviation, numTests);
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

        if (evaluations.size() == 2) {
            // Find the confidence interval for difference in means
        }
    }

   private:
    // Levene's test for the equality of variance, this is incorrect currently
    // returns whether it has equal variances
    bool hasEqualVariances(std::vector<Evaluation>& evaluations, double alpha = 0.95) {
        // https://medium.com/@kyawsawhtoon/levenes-test-the-assessment-for-equality-of-variances-94503b695a57
        // calculate test statistic W
        const int N = std::accumulate(evaluations.begin(), evaluations.end(), 0,
                                      [](int count, const Evaluation& evaluation) {
                                          return count + evaluation.numTests;
                                      });  // Total number of cases in all groups
        const int k =
            evaluations.size();  // Total number of different / sub groups the sampled cases belong

        // transform the data!

        // For each group, sum the (num cases in group * (mean of group - mean of all groups)^2) to
        // get the second parts nominator
        const double allGroupTotal = std::accumulate(
            evaluations.begin(), evaluations.end(), 0, [](int total, const Evaluation& evaluation) {
                // evalMean = evalTotal / evalNumTests, rearranging
                return total + evaluation.meanCycles * evaluation.numTests;
            });
        const double allGroupMean = allGroupTotal / N;  // divide by number of total observations

        const double nominator = std::accumulate(
            evaluations.begin(), evaluations.end(), 0,
            [allGroupMean](double rollingTotal, const Evaluation& eval) {
                return rollingTotal + eval.numTests * std::pow(eval.meanCycles - allGroupMean, 2);
            });

        // For the second parts dominator, sum together (observation - mean of that observations
        // group)^2, for each observation in each group.

        double denominator{};
        for (const Evaluation& eval : evaluations) {
            for (size_t observation : eval.observationValues) {
                denominator += std::pow(observation - eval.meanCycles, 2);
            }
        }

        // the test statistic!
        const double W = static_cast<double>(N - k) / (k - 1) * nominator / denominator;

        // first df is number subgroups - 1, second df is num observations - num of subgroups
        boost::math::fisher_f dist(k - 1, N - k);
        // get the critical value for my alpha and F distribution
        const double criticalF = boost::math::quantile(dist, alpha);

        // If W < than the critical value, we fail to reject the null hypothesis of equal variances
        // so this returns true as it is considered as having equal variances!
        return W < criticalF;
    }

    std::vector<Benchmark> benchmarks;
};

// Takes in a variadic number of benchables
template <typename... Ts>
    requires(std::derived_from<std::remove_cvref_t<Ts>, Benchable> && ...)
void executeBench(size_t iterations, size_t numSamples, Ts&&... benches) {
    BenchmarkCollection collection = BenchmarkCollection();
    Timer& timer = Timer::getInstance();

    auto bench = [&]<typename B>(B& benchable) {
        timer.startTimer();
        benchable.runBenchmark(iterations);
        size_t cyclesTaken = timer.endTimer();

        benchable.resetBenchmark();
        collection.addTime(benchable.getName(), cyclesTaken);
    };

    for (size_t i{}; i < numSamples; ++i) {
        (bench(benches), ...);
    }

    collection.print();
}

}  // namespace benchmarks