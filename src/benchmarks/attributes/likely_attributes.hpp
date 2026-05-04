#pragma once

#include <format>
#include <random>
#include <string>
#include <vector>

#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

enum class Attribute { LIKELY, UNLIKELY, DEFAULT, BRANCHLESS };

// C++ 20 introduces 2 questionably useful attributes [[likely]] and [[unlikely]] which supposedly
// give hints to the compiler about whether a path of execution is more or less likely than another
// pass of execution. The compiler can then optimize with that in mind
template <Attribute A>
class AttributeOptimisation : public Benchable {
   public:
    AttributeOptimisation(size_t listSize) :
            Benchable(BenchType::ATTRIBUTE_BRANCH_PREDICTION,
                      std::format("Compiler Branch Prediction with attribute {}", ATTRIBUTE_NAME)),
            listSize{listSize} {
        resetBenchmark();
    }

    void resetBenchmark() override {
        // get a random seed from the hardware
        std::random_device rd;

        // make a mersenne twister RNG with a distribution for it
        std::mt19937 randomNumberGenerator(rd());
        std::uniform_int_distribution<int> distrib(DISTRIBUTION_MIN, DISTRIBUTION_MAX);

        randomNumbers.resize(listSize);
        for (size_t i{}; i < randomNumbers.size(); ++i) {
            randomNumbers[i] = distrib(randomNumberGenerator);
        }
    }

    void runBenchmark(size_t iterations) override {
        volatile size_t successes{};
        volatile size_t equalities{};
        for (size_t i{}; i < iterations; ++i) {
            for (int number : randomNumbers) {
                // This has a 95% chance for a success here with a uniform distribution. So I'm
                // going to rightfully inform it this path is a likely one.
                if constexpr (A == Attribute::LIKELY) {
                    if (number > SIZE_NEEDED_FOR_SUCCESS) [[likely]] {
                        successes += 1;
                    } else if (number == SIZE_NEEDED_FOR_SUCCESS) [[unlikely]] {
                        equalities += 1;
                    }
                }

                // Lie to compiler that this event is unlikely, could tank performance theoretically
                // if the compiler believes me naively and reorders this branch into something like
                // number == SIZE_NEEDED_FOR_SUCCESS to check the else condition first effectively
                if constexpr (A == Attribute::UNLIKELY) {
                    if (number > SIZE_NEEDED_FOR_SUCCESS) [[unlikely]] {  // lie 95% is unlikely
                        successes += 1;
                    } else if (number == SIZE_NEEDED_FOR_SUCCESS) [[likely]] {  // lie ~0% is likely
                        equalities += 1;
                    }
                }
                // Don't inform the compiler of anything extra
                if constexpr (A == Attribute::DEFAULT) {
                    if (number > SIZE_NEEDED_FOR_SUCCESS) {
                        successes += 1;
                    } else if (number == SIZE_NEEDED_FOR_SUCCESS) {
                        equalities += 1;
                    }
                }

                // A manual branchless implementation
                if constexpr (A == Attribute::BRANCHLESS) {
                    successes += (number > SIZE_NEEDED_FOR_SUCCESS);
                    equalities += (number == SIZE_NEEDED_FOR_SUCCESS);
                }
            }
        }
    }

   private:
    std::vector<int> randomNumbers;
    size_t listSize;
    static constexpr std::string_view ATTRIBUTE_NAME = []() {
        switch (A) {
        case Attribute::LIKELY:
            return "LIKELY";
        case Attribute::UNLIKELY:
            return "UNLIKELY";
        case Attribute::DEFAULT:
            return "DEFAULT BEHAVIOR";
        case Attribute::BRANCHLESS:
            return "BRANCHLESS VERSION";
        }
        return "FALLTHROUGH ISSUE";
    }();

    static constexpr int DISTRIBUTION_MAX{100000};
    static constexpr int DISTRIBUTION_MIN{0};
    // 95% chance for a success here with a uniform distribution
    static constexpr int SIZE_NEEDED_FOR_SUCCESS{(DISTRIBUTION_MAX + DISTRIBUTION_MIN) / 20 * 6};
};

}  // namespace benchmarks