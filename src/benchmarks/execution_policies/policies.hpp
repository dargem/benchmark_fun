#pragma once

#include <execution>
#include <format>
#include <random>
#include <string>
#include <vector>

#include "benchmarks/bench_types.hpp"
#include "benchmarks/benchable.hpp"

namespace benchmarks {

enum class Policy {
    SEQUENCED,            // Iterates in sequence
    UNSEQUENCED,          // Allows unsequenced iteration (can do vectorization/simd)
    PARALLEL,             // Allows parallel iteration (compiler can choose to do multithreading)
    PARALLEL_UNSEQUENCED  // Allows both parallel and unsequenced iteration (simd + multithreading)
};

template <Policy P>
class ExecutionPolicies : public Benchable {
   public:
    ExecutionPolicies(size_t coordinateSize) :
            Benchable(BenchType::EXECUTION_POLICY,
                      std::format("{} Execution Policy Benchmark", TYPE)),
            x_pos(coordinateSize, float{}),
            y_pos(coordinateSize, float{}),
            z_pos(coordinateSize, float{}) {
        asm volatile("" : : "r"(x_pos.data()) : "memory");
        asm volatile("" : : "r"(y_pos.data()) : "memory");
        asm volatile("" : : "r"(z_pos.data()) : "memory");

        std::random_device rd;
        std::mt19937 generator(rd());
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
        // split vector into its individual transformations
        const float x_transform = std::get<0>(transformationVector);
        const float y_transform = std::get<1>(transformationVector);
        const float z_transform = std::get<2>(transformationVector);

        for (size_t i{}; i < iterations; ++i) {
            if constexpr (P == Policy::SEQUENCED) {
                std::for_each(std::execution::seq, x_pos.begin(), x_pos.end(),
                              [x_transform](float& x_coord) { x_coord += x_transform; });
                std::for_each(std::execution::seq, y_pos.begin(), y_pos.end(),
                              [y_transform](float& y_coord) { y_coord += y_transform; });
                std::for_each(std::execution::seq, z_pos.begin(), z_pos.end(),
                              [z_transform](float& z_coord) { z_coord += z_transform; });
            }
            if constexpr (P == Policy::UNSEQUENCED) {
                std::for_each(std::execution::unseq, x_pos.begin(), x_pos.end(),
                              [x_transform](float& x_coord) { x_coord += x_transform; });
                std::for_each(std::execution::unseq, y_pos.begin(), y_pos.end(),
                              [y_transform](float& y_coord) { y_coord += y_transform; });
                std::for_each(std::execution::unseq, z_pos.begin(), z_pos.end(),
                              [z_transform](float& z_coord) { z_coord += z_transform; });
            }
            if constexpr (P == Policy::PARALLEL) {
                std::for_each(std::execution::par, x_pos.begin(), x_pos.end(),
                              [x_transform](float& x_coord) { x_coord += x_transform; });
                std::for_each(std::execution::par, y_pos.begin(), y_pos.end(),
                              [y_transform](float& y_coord) { y_coord += y_transform; });
                std::for_each(std::execution::par, z_pos.begin(), z_pos.end(),
                              [z_transform](float& z_coord) { z_coord += z_transform; });
            }
            if constexpr (P == Policy::PARALLEL_UNSEQUENCED) {
                std::for_each(std::execution::par_unseq, x_pos.begin(), x_pos.end(),
                              [x_transform](float& x_coord) { x_coord += x_transform; });
                std::for_each(std::execution::par_unseq, y_pos.begin(), y_pos.end(),
                              [y_transform](float& y_coord) { y_coord += y_transform; });
                std::for_each(std::execution::par_unseq, z_pos.begin(), z_pos.end(),
                              [z_transform](float& z_coord) { z_coord += z_transform; });
            }
        }
    }

   private:
    static constexpr std::string_view TYPE = []() {
        switch (P) {
        case Policy::SEQUENCED:
            return "Sequenced";
        case Policy::UNSEQUENCED:
            return "Unsequenced";
        case Policy::PARALLEL:
            return "Parallel";
        case Policy::PARALLEL_UNSEQUENCED:
            return "Parallel and Unsequenced";
        }
        return "fallback, should never see this";
    }();
    std::tuple<float, float, float> transformationVector;  // x, y, z vector
    // SOA coordinates
    std::vector<float> x_pos;
    std::vector<float> y_pos;
    std::vector<float> z_pos;
};

}  // namespace benchmarks