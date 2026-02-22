#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

#include "benchmarks/bench_types.hpp"
#include "benchmarks/benchable.hpp"

namespace benchmarks {

// a generic vector access class that can be specialized for chars, bools or uint8_t
template <typename T>
    requires std::same_as<T, char> || std::same_as<T, bool> || std::same_as<T, uint8_t>
class VectorAccess : public Benchable {
   public:
    VectorAccess(size_t numElements, size_t numIndices) :
            Benchable(benchType, name),
            rng(10000),
            elements(numElements, T{}),
            indices(numIndices) {
        std::uniform_int_distribution<size_t> dist(0, numElements - 1);

        for (auto& i : indices) {
            i = dist(rng);  // initialise the indices with indexes of elements
        }
    }

    void runBenchmark(size_t iterations) override {
        T sink{};  // prevent dead code elimination
        if constexpr (std::same_as<T, bool>) {
            asm volatile("" : : "r"(elements.size()) : "memory");
        } else {
            asm volatile("" : : "r"(elements.data()) : "memory");
        }

        for (size_t i{}; i < iterations; ++i) {
            for (size_t index : indices) {
                sink ^= elements[index];
            }
        }

        asm volatile("" : : "r"(sink) : "memory");
    }

    void resetBenchmark() override {
        std::uniform_int_distribution<size_t> dist(0, elements.size() - 1);

        for (auto& i : indices) {
            i = dist(rng);  // initialise the indices with indexes of elements
        }
    }

   private:
    std::mt19937 rng;  // mersenne twister RNG
    std::vector<T> elements;
    std::vector<size_t> indices;

    constexpr static BenchType benchType{BenchType::VECTOR_BOOL_CHAR_RANDOM_ACCESS};
    static constexpr std::string_view name = []() {
        if constexpr (std::same_as<T, bool>)
            return "bool vector random access";
        else if constexpr (std::same_as<T, char>)
            return "char vector random access";
        else if constexpr (std::same_as<T, uint8_t>)
            return "uint8_t vector random access";
        else
            return "unknown vector random access";
    }();
};

}  // namespace benchmarks