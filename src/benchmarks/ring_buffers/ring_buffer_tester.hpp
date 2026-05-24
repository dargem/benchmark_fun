#pragma once

#include <pthread.h>
#include <sched.h>

#include <atomic>
#include <concepts>
#include <cstdlib>
#include <format>
#include <stdexcept>
#include <thread>

#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

template <typename Queue>
    requires requires(Queue q, int a) {  // Beautiful concepts syntax
        q.push(a);
        q.pop(a);
        q.reset();
        { Queue::NAME } -> std::convertible_to<std::string_view>;
    }
class BufferTester : public Benchable {
   public:
    BufferTester(size_t queue_size) :
            Benchable(std::format("{} benchmark", std::string(Queue::NAME))), q(queue_size) {}

    void runBenchmark(size_t iterations) override {
        std::atomic<int> ready{0};
        std::atomic<bool> start{false};

        // Can set environmental variables to avoid recompiling
        const int popCpu = getEnvCpu("BENCH_POP_CPU", 0);
        const int pushCpu = getEnvCpu("BENCH_PUSH_CPU", 3);

        popper = std::thread([&, iterations] {
            pinThread(popCpu);
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire));

            for (int i{}; i < iterations; ++i) {
                int val;
                // If pop fails keep on busy waiting until success
                while (!q.pop(val));

                // if (val != i) {
                //     throw std::runtime_error(
                //         std::format("Queue returned wrong value: expected {}, got {}", i, val));
                // } // doesn't throw when uncommented so its working
            }
        });

        pusher = std::thread([&, iterations] {
            pinThread(pushCpu);
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire));

            for (int i{}; i < iterations; ++i) {
                // Keep on trying to push index busily
                while (!q.push(i));
            }
        });

        while (ready.load(std::memory_order_acquire) != 2);
        start.store(true, std::memory_order_release);  // Start trigger

        popper.join();
        pusher.join();
    }

    void resetBenchmark() override { q.reset(); }

   private:
    static int getEnvCpu(const char* name, int fallback) {
        const char* value = std::getenv(name);
        if (value == nullptr) return fallback;
        char* end = nullptr;
        long parsed = std::strtol(value, &end, 10);
        if (end == value || *end != '\0') return fallback;
        if (parsed < 0) return fallback;
        if (parsed >= CPU_SETSIZE) return fallback;
        return static_cast<int>(parsed);
    }

    void pinThread(int cpu) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
            perror("pthread_setaffinity_no");
            exit(1);
        }
    }
    std::thread popper;
    std::thread pusher;
    Queue q;
};

}  // namespace benchmarks