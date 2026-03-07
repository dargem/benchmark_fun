#pragma once

#include <cmath>
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

        const auto heavyTransform = [](float& coord, float transform) {
            float v = coord;
            for (size_t k{}; k < 4; ++k) {
                v = std::sin(v + transform) + std::cos(v);
            }
            coord = v;
        };

        const auto applyForEach = [&](auto execPolicy, std::vector<float>& coords,
                                      float transform) {
            std::for_each(
                execPolicy, coords.begin(), coords.end(),
                [transform, &heavyTransform](float& coord) { heavyTransform(coord, transform); });
        };

        for (size_t i{}; i < iterations; ++i) {
            if constexpr (P == Policy::SEQUENCED) {
                applyForEach(std::execution::seq, x_pos, x_transform);
                applyForEach(std::execution::seq, y_pos, y_transform);
                applyForEach(std::execution::seq, z_pos, z_transform);
            } else if constexpr (P == Policy::UNSEQUENCED) {
                applyForEach(std::execution::unseq, x_pos, x_transform);
                applyForEach(std::execution::unseq, y_pos, y_transform);
                applyForEach(std::execution::unseq, z_pos, z_transform);
            } else if constexpr (P == Policy::PARALLEL) {
                applyForEach(std::execution::par, x_pos, x_transform);
                applyForEach(std::execution::par, y_pos, y_transform);
                applyForEach(std::execution::par, z_pos, z_transform);
            } else if constexpr (P == Policy::PARALLEL_UNSEQUENCED) {
                applyForEach(std::execution::par_unseq, x_pos, x_transform);
                applyForEach(std::execution::par_unseq, y_pos, y_transform);
                applyForEach(std::execution::par_unseq, z_pos, z_transform);
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