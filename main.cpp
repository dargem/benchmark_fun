#include "src/timer.hpp"
#include "src/benchmarks/rng/xoroshiro128+.hpp"
#include <iostream>

using benchmarks::Xoroshiro128plus;

int main() {
    constexpr int ITERATIONS{1000000};
    Xoroshiro128plus<true> rng{ 50000 };
    //rng.printNextDouble();
    Timer& timer = Timer::getInstance();

    timer.startTimer();
    rng.runBenchmark(ITERATIONS);
    auto cycles = timer.endTimer();
    std::cout << "Cycles: " << cycles << std::endl;
    rng.printNextDouble(); // observe the output
    return 0;
}