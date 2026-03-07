#pragma once

#include <format>
#include <random>
#include <string>
#include <vector>

#include "benchmarks/bench_types.hpp"
#include "benchmarks/benchable.hpp"

namespace benchmarks {

enum class ExecutionPolicy {
    SEQUENCED,            // Iterates in sequence
    UNSEQUENCED,          // Allows unsequenced iteration (can do vectorization/simd)
    PARALLEL,             // Allows parallel iteration (compiler can choose to do multithreading)
    PARALLEL_UNSEQUENCED  // Allows both parallel and unsequenced iteration (simd + multithreading)
};

template <ExecutionPolicy P>
class ExecutionPolicyBenchmark : public Benchable {
   public:
    ExecutionPolicyBenchmark(size_t coordinateSize) :
            Benchable(BenchType::EXECUTION_POLICY, NAME),
            x_pos(coordinateSize, float{}),
            y_pos(coordinateSize, float{}),
            z_pos(coordinateSize, float{}) {
        asm volatile("" : : "r"(x_pos.data()) : "memory");
        asm volatile("" : : "r"(y_pos.data()) : "memory");
        asm volatile("" : : "r"(z_pos.data()) : "memory");

        std::random_device rd;
        std::mt19937 generator(rd);
        std::uniform_real_distribution<float> distribution(-10, 10);

        std::get<0>(transformationVector) = distribution(generator);
        std::get<1>(transformationVector) = distribution(generator);
        std::get<2>(transformationVector) = distribution(generator);
    }

    void resetBenchmark() override {
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_real_distribution<float> distribution(-10, 10);

        std::get<0>(transformationVector) = distribution(generator);
        std::get<1>(transformationVector) = distribution(generator);
        std::get<2>(transformationVector) = distribution(generator);
    }

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            if constexpr (P == ExecutionPolicy::SEQUENCED) {
            }
            if constexpr (P == ExecutionPolicy::UNSEQUENCED) {
            }
            if constexpr (P == ExecutionPolicy::PARALLEL) {
            }
            if constexpr (P == ExecutionPolicy::PARALLEL_UNSEQUENCED) {
            }
        }
    }

   private:
    static constexpr std::string NAME =
        std::format("{} Execution Policy bench", []() -> std::string {
            switch (P) {
            case ExecutionPolicy::SEQUENCED:
                return "Sequenced";
            case ExecutionPolicy::UNSEQUENCED:
                return "Unsequenced";
            case ExecutionPolicy::PARALLEL:
                return "Parallel";
            case ExecutionPolicy::PARALLEL_UNSEQUENCED:
                return "Parallel and Unsequenced";
            }
            return "fallback, should never see this";
        });
    std::tuple<float, float, float> transformationVector;  // x, y, z vector
    // SOA coordinates
    std::vector<float> x_pos;
    std::vector<float> y_pos;
    std::vector<float> z_pos;
};

}  // namespace benchmarks