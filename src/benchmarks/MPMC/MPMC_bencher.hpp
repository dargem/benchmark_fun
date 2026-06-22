#pragma once

#include <pthread.h>
#include <sched.h>

#include <atomic>
#include <concepts>
#include <format>
#include <string_view>
#include <thread>

#include "benchmarks/benchable.hpp"

namespace benchmarks {

template <typename Queue>
    requires requires(Queue q, int a) {  // Beautiful concepts syntax
        q.push(a);
        q.pop(a);
        q.reset();
        { Queue::NAME } -> std::convertible_to<std::string_view>;
    }
class MPMCQueueTester : public Benchable {
   public:
    MPMCQueueTester(size_t queueSize) :
            Benchable(std::format("{} benchmark", std::string(Queue::NAME)), q(queueSize)) {}

    void runBenchmark(size_t iterations) override {
        std::atomic<int> ready{};
        std::atomic<bool> start{false};

        pinThread(0);

        auto pop = [&](int cpu) {
            pinThread(cpu);
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire));

            for (size_t i{}; i < iterations; ++i) {
                int val;
                while (!q.pop(val));
            }
        };

        auto push = [&](int cpu) {
            ready.fetch_add(1, std::memory_order_release);
            pinThread(cpu);
            while (!start.load(std::memory_order_acquire));

            for (size_t i{}; i < iterations; ++i) {
                while (!q.push(i));
            }
        };

        popper0 = std::thread(pop(2));
        popper1 = std::thread(pop(4));
        popper2 = std::thread(pop(6));
        pusher0 = std::thread(push(8));
        pusher1 = std::thread(push(10));
        pusher2 = std::thread(push(12));

        while (ready.load(std::memory_order_acquire) != 6);
        start.store(true, std::memory_order_release);  // Start trigger

        popper0.join();
        popper1.join();
        popper2.join();
        pusher0.join();
        pusher1.join();
        pusher2.join();
    }

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

    std::thread popper0;
    std::thread popper1;
    std::thread popper2;
    std::thread pusher0;
    std::thread pusher1;
    std::thread pusher2;
    Queue q;
};

}  // namespace benchmarks