#pragma once

#include <concepts>
#include <format>
#include <stdexcept>
#include <thread>

#include "src/benchmarks/benchable.hpp"
#include "src/benchmarks/ring_buffers/cachy.hpp"
#include "src/benchmarks/ring_buffers/classic.hpp"
#include "src/benchmarks/ring_buffers/ring_buffer_tester.hpp"

namespace benchmarks {

template <typename Queue>
    requires requires(Queue q, int a) {  // Beautiful concepts syntax
        q.push(a);
        q.pop();
        { q.name } -> std::convertible_to<std::string>;
    }
class BufferTester : public Benchable {
   public:
    BufferTester(size_t queue_size) :
            Benchable(std::format("{} benchmark", std::declval<Queue>().name)), q() {}

    void runBenchmark(size_t iterations) override {
        popper([&, iterations] {
            for (int i{}; i < iterations; ++i) {
                int val;
                // If pop fails keep on busy waiting until success
                while (!q.pop());
                if (val != i) {
                    throw std::runtime_error("issue");
                }
            }
        });

        pusher([&, iterations] {
            for (int i{}; i < iterations; ++i) {
                // Keep on trying to push index busily
                while (!q.push(i));
            }
        });

        popper.join();
        pusher.join();
    }

    void resetBenchmark() override {}

   private:
    std::thread popper;
    std::thread pusher;
    Queue q;
};

}  // namespace benchmarks