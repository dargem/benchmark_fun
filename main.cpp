#include "src/timer.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include "src/benchmarks/bench_runner.hpp"
#include <iostream>
#include <memory>

using benchmarks::Xoroshiro128plus;
using benchmarks::BenchRunner;
using benchmarks::Benchable;


int main() {
    try {
        BenchRunner& benchRunner = BenchRunner::getInstance();    


        constexpr size_t ITERATIONS{ 1000000 };
        constexpr size_t NUM_SAMPLES{ 50 };

        
        constexpr int RNG_SEED{ 50000 };

        std::unique_ptr<Benchable> rng = std::make_unique<Xoroshiro128plus<true>>(RNG_SEED);
        
        
        constexpr size_t LIST_SIZE{ 100000 };

        benchRunner.addBenchable(std::move(rng));
        benchRunner.runBenchmarks(ITERATIONS, NUM_SAMPLES);
        benchRunner.printResults();
        


    } catch(const std::runtime_error& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    return 0;
}