#pragma once

#include <format>
#include <string>
#include <vector>

#include "src/benchmarks/bench_types.hpp"
#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

template <typename T>
// for float types
concept Floating =
    std::same_as<T, float> || std::same_as<T, double> || std::same_as<T, long double>;

template <Floating T>
class AOS : public Benchable {
   private:
    struct Entity {
        T attack{1.0};
        T defense{1.0};
        T health{1.0};
        T x{0.0};
        T y{0.0};
        T z{0.0};
    };

   public:
    AOS(size_t numEntities) :
            Benchable(BenchType::STRUCTURE_LAYOUT,
                      std::format("Array of Structures Iteration over {}", typeName)),
            entities(numEntities, Entity{}) {}

    void runBenchmark(size_t iterations) override {
        asm volatile("" : : "r"(entities.data()) : "memory");
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
    constexpr static std::string_view typeName = []() {
        if constexpr (std::same_as<T, float>) {
            return "float";
        } else if constexpr (std::same_as<T, double>) {
            return "double";
        } else if constexpr (std::same_as<T, long double>) {
            return "long double";
        } else {
            // this would fail prior to c++ 23 interestingly
            static_assert(false, "Fallthrough on getting type name in structures.hpp");
            return "holder";
        }
    }();
};

template <Floating T>
class SOA : public Benchable {
   public:
    SOA(size_t numEntities) :
            Benchable(BenchType::STRUCTURE_LAYOUT, "Structure of Arrays iteration"),
            attacks(numEntities, 1.0),
            defenses(numEntities, 1.0),
            healths(numEntities, 1.0),
            x_locs(numEntities, {}),
            y_locs(numEntities, {}),
            z_locs(numEntities, {}) {}

    void runBenchmark(size_t iterations) override {
        asm volatile("" : : "r"(attacks.data()) : "memory");
        asm volatile("" : : "r"(defenses.data()) : "memory");
        asm volatile("" : : "r"(healths.data()) : "memory");
        asm volatile("" : : "r"(x_locs.data()) : "memory");
        asm volatile("" : : "r"(y_locs.data()) : "memory");
        asm volatile("" : : "r"(z_locs.data()) : "memory");
        // bland iteration, translating x, y and z by 1 in each direction
        // should be faster then AOS maybe due to SIMD?
        for (size_t i{}; i < iterations; ++i) {
            for (T& x : x_locs) {
                x += 1;
            }

            for (T& y : y_locs) {
                y += 1;
            }

            for (T& z : z_locs) {
                z += 1;
            }
        }
    }

    void resetBenchmark() override {
        // nothing to really do on reset, just pass this
    }

   private:
    std::vector<T> attacks;
    std::vector<T> defenses;
    std::vector<T> healths;
    std::vector<T> x_locs;
    std::vector<T> y_locs;
    std::vector<T> z_locs;
};

}  // namespace benchmarks