#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace benchmarks {

// base class for benchmarks
class Benchable {
   public:
    explicit Benchable(std::string name) : name{std::move(name)} {}

    // call when reusing a benchmark, assume benchables will need some setup
    virtual void resetBenchmark() = 0;
    // call to run the benchmark for the number of iterations
    virtual void runBenchmark(size_t iterations) = 0;

    bool operator==(const Benchable& other) const { return name == other.name; }

    std::string_view getName() const { return name; }

   private:
    const std::string name;
};

}  // namespace benchmarks