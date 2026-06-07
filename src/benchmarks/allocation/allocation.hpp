#pragma once

#include <cstddef>

namespace benchmarks {

template <typename Allocator>
concept IsAllocator = requires(Allocator t, size_t n) { t.template allocate<int>(n); };

template <typename Allocator>
    requires IsAllocator<Allocator>
class AllocationBench {};

}  // namespace benchmarks