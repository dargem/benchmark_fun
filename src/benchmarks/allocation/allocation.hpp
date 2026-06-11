#pragma once

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

#include "benchmarks/allocation/arena.hpp"
#include "benchmarks/allocation/new.hpp"
#include "benchmarks/benchable.hpp"

namespace benchmarks {

enum class Allocator { ARENA, NEW };

template <Allocator A>
struct AllocatorTraits {};

template <>
struct AllocatorTraits<Allocator::ARENA> {
    using A = Arena<true>;
};

template <>
struct AllocatorTraits<Allocator::NEW> {
    using A = NewWrapper;
};

template <Allocator A, bool BenchesDeletion = false>
class AllocationBench : public Benchable {
    static constexpr std::string_view name = [] {
        if constexpr (A == Allocator::ARENA) return "Arena Allocator";
        if constexpr (A == Allocator::NEW) return "New Allocator";
        return "fallback";
    }();

   public:
    AllocationBench() : Benchable(std::forward<std::string>(std::string(name))), allocator() {}
    AllocationBench(const AllocationBench&) = delete;
    AllocationBench operator=(const AllocationBench&) = delete;

    void runBenchmark(size_t iterations) override {
        a.reserve(iterations);
        for (size_t i{}; i < iterations; ++i) {
            a.push_back(allocator.template allocate<int>());
        }

        if constexpr (BenchesDeletion == true) {
            if constexpr (A == Allocator::ARENA) {
                allocator.reset();
            } else {
                for (int* p : a) {
                    delete p;
                }
            }
        }
    }

    void resetBenchmark() override {
        if constexpr (BenchesDeletion == false) {
            if constexpr (A == Allocator::ARENA) {
                allocator.reset();
            } else {
                for (int* p : a) {
                    delete p;
                }
            }
        }
        a.resize(0);
    }

   private:
    std::vector<int*> a;
    AllocatorTraits<A>::A allocator;
};

}  // namespace benchmarks