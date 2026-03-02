#pragma once

#include <vector>

#include "src/benchmarks/bench_types.hpp"
#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

namespace {
// Entity types
struct Entity {
    double attack{1.0};
    double defense{1.0};
    double health{1.0};
    double x{};
    double y{};
    double z{};
};

}  // namespace

class AOS : public Benchable {
   public:
    AOS(size_t numEntities) :
            Benchable(BenchType::STRUCTURE_LAYOUT, "Array of Structures Iteration"),
            entities(numEntities, Entity{}) {}

    void runBenchmark(size_t iterations) override {
        asm volatile("" : : "r"(entities) : "memory");
        // bland iteration, translating x, y and z by 1 in each direction
        for (size_t i{}; i < iterations; ++i) {
            for (Entity& entity : entities) {
                entity.x += 1;
                entity.y += 1;
                entity.z += 1;
            }
        }
    }

    void resetBenchmark() override {
        // nothing to really do on reset, just pass this
    }

   private:
    std::vector<Entity> entities;  // Entity types
};

class SOA : public Benchable {
   public:
    SOA(size_t numEntities) :
            Benchable(BenchType::STRUCTURE_LAYOUT, "Structure of Array iteration"),
            attacks(numEntities, 1.0),
            defenses(numEntities, 1.0),
            healths(numEntities, 1.0),
            x_locs(numEntities, {}),
            y_locs(numEntities, {}),
            z_locs(numEntities, {}) {}

    void runBenchmark(size_t iterations) override {
        asm volatile("" : : "r"(attacks) : "memory");
        asm volatile("" : : "r"(defenses) : "memory");
        asm volatile("" : : "r"(healths) : "memory");
        asm volatile("" : : "r"(x_locs) : "memory");
        asm volatile("" : : "r"(y_locs) : "memory");
        asm volatile("" : : "r"(z_locs) : "memory");
        // bland iteration, translating x, y and z by 1 in each direction
        // should be faster then SOA maybe due to SIMD?
        for (size_t i{}; i < iterations; ++i) {
            for (double& x : x_locs) {
                x += 1;
            }

            for (double& y : y_locs) {
                y += 1;
            }

            for (double& z : z_locs) {
                z += 1;
            }
        }
    }

    void resetBenchmark() override {
        // nothing to really do on reset, just pass this
    }

   private:
    std::vector<double> attacks;
    std::vector<double> defenses;
    std::vector<double> healths;
    std::vector<double> x_locs;
    std::vector<double> y_locs;
    std::vector<double> z_locs;
};

}  // namespace benchmarks