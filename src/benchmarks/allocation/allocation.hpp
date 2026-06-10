#pragma once

#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

#include "benchmarks/benchable.hpp"

namespace benchmarks {

template <typename Allocator>
concept IsAllocator = requires(Allocator t, size_t n) {
    { t.template allocate<int>(n) } -> std::same_as<int*>;
    Allocator();
    t.reset();
};

template <typename Allocator>
    requires IsAllocator<Allocator>
class AllocationBench : public Benchable {
   public:
    AllocationBench(std::string&& name) : Benchable(std::forward<std::string>(name)), allocator() {}
    AllocationBench(const AllocationBench&) = delete;
    AllocationBench operator=(const AllocationBench&) = delete;

    void runBenchmark(size_t iterations) override {
        a.reserve(iterations);
        for (size_t i{}; i < iterations; ++i) {
            a.push_back(allocator.template allocate<int>(1));
        }
    }

    void resetBenchmark() override {
        allocator.reset();
        a.resize(0);
    }

   private:
    std::vector<int*> a;
    Allocator allocator;
};

}  // namespace benchmarks