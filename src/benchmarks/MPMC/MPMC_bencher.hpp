#pragma once

#include <pthread.h>
#include <sched.h>

#include <atomic>
#include <concepts>
#include <format>
#include <string_view>
#include <thread>
#include <vector>

#include "benchmarks/benchable.hpp"

namespace benchmarks {

template <typename Queue>
    requires requires(Queue q, int a, size_t b) {  // Beautiful concepts syntax
        { q.push(a) } -> std::convertible_to<bool>;
        { q.pop(a) } -> std::convertible_to<bool>;
        q.reset();
        { Queue::NAME } -> std::convertible_to<std::string_view>;
    }
class MPMCQueueTester : public Benchable {
   public:
    MPMCQueueTester(size_t queueSize, size_t numPairs) :
            Benchable(std::format("{} benchmark for {} consumers and {} producers",
                                  std::string(Queue::NAME), numPairs, numPairs)),
            q(queueSize),
            numProducers(numPairs),
            numConsumers((numPairs)) {}

    void runBenchmark(size_t iterations) override {
        std::atomic<int> ready{0};
        std::atomic<bool> start{false};

        // pinThread(0);

        auto popFn = [&](int cpu) {
            // pinThread(cpu);
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire));

            for (size_t i = 0; i < iterations; ++i) {
                int val;
                while (!q.pop(val));
            }
        };

        auto pushFn = [&](int cpu) {
            // pinThread(cpu);
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire));

            for (size_t i = 0; i < iterations; ++i) {
                while (!q.push(i));
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(numProducers + numConsumers);

        int cpu = 0;

        // On my laptop pairs of sequential logical cores maps to pairs of physical cores
        for (size_t i = 0; i < numConsumers; ++i) threads.emplace_back(popFn, (cpu += 2) % 24);
        for (size_t i = 0; i < numProducers; ++i) threads.emplace_back(pushFn, (cpu += 2) % 24);

        const int totalThreads = static_cast<int>(numProducers + numConsumers);
        while (ready.load(std::memory_order_acquire) != totalThreads);
        start.store(true, std::memory_order_release);

        for (auto& t : threads) t.join();
    }

    void resetBenchmark() override { q.reset(); }

   private:
    void pinThread(int cpu) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
            perror("pthread_setaffinity_no");
            exit(1);
        }
    }

    std::vector<std::thread> threads;
    Queue q;
    const size_t numProducers;
    const size_t numConsumers;
};

}  // namespace benchmarks