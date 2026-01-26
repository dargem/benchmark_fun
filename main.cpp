#include "src/timer.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include <iostream>

using benchmarks::Xoroshiro128plus;

int main() {
    constexpr int ITERATIONS{1000000};
    Xoroshiro128plus rng{ 50000 };
    Timer& timer = Timer::getInstance();

    timer.startTimer();
    rng.runBenchmark(ITERATIONS);
    std::cout << "Cycles: " << timer.endTimer() << std::endl;
    return 0;
}