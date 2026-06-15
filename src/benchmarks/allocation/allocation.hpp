#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include "benchmarks/allocation/arena.hpp"
#include "benchmarks/allocation/new.hpp"
#include "benchmarks/benchable.hpp"
#include "utils/xoroshiro_64_star.hpp"

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

template <Allocator A, bool BenchesDeletion = false, bool VariableAllocSizes = false>
class AllocationBench : public Benchable {
    static constexpr std::string_view name = [] {
        if constexpr (A == Allocator::ARENA) return "Arena Allocator";
        if constexpr (A == Allocator::NEW) return "New Allocator";
        return "fallback";
    }();

   public:
    AllocationBench(size_t n_elements) :
            Benchable(std::forward<std::string>(std::string(name))),
            allocator(),
            elements(n_elements),
            rng() {
        if constexpr (VariableAllocSizes) {
            sizes.resize(n_elements);
            rng.fill_partial_aligned_uint32(sizes.data(), n_elements);
            for (uint32_t& size : sizes) {
                size >>= 24;  // get 8 bits [0, 255] bytes
                size += 1;    // To never get 0 byte allocation
            }
        }
    }
    AllocationBench(const AllocationBench&) = delete;
    AllocationBench operator=(const AllocationBench&) = delete;

    void runBenchmark(size_t iterations) override {
        a.reserve(iterations);
        for (size_t i{}; i < iterations; ++i) {
            if constexpr (VariableAllocSizes == true) {
                for (uint32_t size : sizes) {
                    a.push_back(allocator.template allocate<std::byte>(size));
                }
            } else {
                for (size_t i{}; i < elements; ++i) {
                    a.push_back(allocator.template allocate<std::byte>(4));
                }
            }
        }

        if constexpr (BenchesDeletion == true) {
            if constexpr (A == Allocator::ARENA) {
                allocator.reset();
            } else {
                for (std::byte* p : a) {
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
                for (std::byte* p : a) {
                    delete p;
                }
            }
        }
        a.resize(0);

        if constexpr (VariableAllocSizes == true) {
            rng.fill_partial_aligned_uint32(sizes.data(), sizes.size());
            for (uint32_t& size : sizes) {
                size >>= 24;  // get 8 bits [0, 255]
                size += 1;    // to never request 0 byte allocation
            }
        }
    }

   private:
    std::vector<std::byte*> a;
    AllocatorTraits<A>::A allocator;
    size_t elements;
    utils::XoroshiroRNG rng;
    std::vector<uint32_t> sizes;
};

}  // namespace benchmarks