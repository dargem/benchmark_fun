#pragma once

#include <cassert>
#include <format>
#include <string>

#include "src/benchmarks/bench_types.hpp"
#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

static_assert(sizeof(std::string) == 32);

template <size_t N>
class StringRunner : public Benchable {
   public:
    StringRunner() :
            Benchable(BenchType::SMALL_STRING_OPTIMISATION, std::format("String of size {}", N)) {}

    void resetBenchmark() override {}

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            std::string str;
            str = string;
            asm volatile("" : : "r"(str.data()) : "memory");
        }
    }

   private:
    std::string string = [] {
        std::string str = "";
        for (size_t i{}; i + 1 < N; ++i) {
            str += " ";  // + 1 to keep in mind null termination
        }
        assert(str.size() + 1 == N);
        return str;
    }();
};

}  // namespace benchmarks