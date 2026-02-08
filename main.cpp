#include "src/timer.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include "src/benchmarks/bench_runner.hpp"
#include <iostream>
#include <memory>

using benchmarks::Xoroshiro128plus;
using benchmarks::BenchRunner;
using benchmarks::Benchable;


int main() {
    BenchRunner& benchRunner = BenchRunner::getInstance();    


    constexpr int ITERATIONS{ 1000000 };
    constexpr int RNG_SEED{ 50000 };

    std::unique_ptr<Benchable> rng = std::make_unique<Xoroshiro128plus<true>>(RNG_SEED);
    benchRunner.addBenchable(std::move(rng));

    return 0;
}