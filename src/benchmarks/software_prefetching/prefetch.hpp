#pragma once

#include <format>

#include "benchmarks/benchable.hpp"

namespace benchmarks {

/*
The way this benchmark works is I have some collision pairs (id's) mapping into an array. Lets
assume this is a game engine or something, the octree has returned potentially overlapping pairs.
Now I need to do a) narrowphase detection to see they actually overlap and b) if so do collision
resolution.
*/
template <bool PrefetchEnabled>
class SoftwarePrefetchBench : public Benchable {
    SoftwarePrefetchBench(size_t numElements) :
            Benchable(std::format("{} Benchmark", std::string(name))), numElements(numElements) {}

    void runBenchmark(size_t iterations) override {}

   private:
    const size_t numElements;
    std::vector<
    static constexpr std::string_view name =
        PrefetchEnabled ? "Software Prefetching" : "Normal Prefetching";
};

}  // namespace benchmarks