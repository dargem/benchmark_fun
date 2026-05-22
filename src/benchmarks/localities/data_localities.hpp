#pragma once

#include <format>
#include <vector>

#include "src/benchmarks/benchable.hpp"
#include "src/utils/xoroshiro_64_star.hpp"

namespace benchmarks {

enum class LocalityType { Data, Instruction };

template <LocalityType T>
class Localities : public Benchable {
   private:
    static constexpr std::string type = []() {
        if constexpr (T == LocalityType::Data) return "Data";
        if constexpr (T == LocalityType::Instruction) return "Instruction";
        return "Missing name mapping";
    }();

   public:
    Localities(size_t numElements) : Benchable(std::format("{} locality benchmark", type)) {
        vec.resize(numElements);
        XoroshiroRNG rng{};
    }

    void runBenchmark(size_t iterations) override {}

    void resetBenchmark() override {}

   private:
    std::vector<float> vec;
};

}  // namespace benchmarks