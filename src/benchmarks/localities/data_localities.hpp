#pragma once

#include <format>
#include <vector>

#include "src/benchmarks/benchable.hpp"
#include "src/utils/xoroshiro_64_star.hpp"
#include "utils/aligned_allocator.hpp"

namespace benchmarks {

enum class LocalityType { Data, Instruction };

template <LocalityType T, size_t LOOP_UNROLL>
class Localities : public Benchable {
   private:
    static constexpr std::string type = []() {
        if constexpr (T == LocalityType::Data) return "Data";
        if constexpr (T == LocalityType::Instruction) return "Instruction";
        return "Missing name mapping";
    }();

   public:
    Localities(size_t numElements) : Benchable(std::format("{} locality benchmark", type)) {
        utils::XoroshiroRNG rng{};
        health.resize(numElements);
        defense.resize(numElements);
        incomingDamage.resize(numElements);
        attack.resize(numElements);
        x_pos.resize(numElements);
        y_pos.resize(numElements);
        z_pos.resize(numElements);

        rng.fill_aligned_uint32(health.data(), numElements);
        rng.fill_aligned_uint32(defense.data(), numElements);
        rng.fill_aligned_uint32(incomingDamage.data(), numElements);
        rng.fill_aligned_uint32(attack.data(), numElements);
        rng.fill_aligned_float(x_pos.data(), numElements);
        rng.fill_aligned_float(y_pos.data(), numElements);
        rng.fill_aligned_float(z_pos.data(), numElements);
    }

    void runBenchmark(size_t iterations) override {
        auto applyDamage = [](uint32_t& health, uint32_t defense, uint32_t incomingDamage) {
            health -= (incomingDamage / defense);
        };

        auto

            for (size_t i{}; i < iterations; ++i) {
            // Should just use 1 iteration
            if constexpr (T == LocalityType::Instruction) {
                // We loop through each system
            }
        }
    }

    void resetBenchmark() override {}

   private:
    std::vector<int, utils::AlignedAllocator<int, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>> health;
    std::vector<uint32_t,
                utils::AlignedAllocator<uint32_t, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>>
        defense;
    std::vector<uint32_t,
                utils::AlignedAllocator<uint32_t, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>>
        incomingDamage;
    std::vector<uint32_t,
                utils::AlignedAllocator<uint32_t, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>>
        attack;
    std::vector<float, utils::AlignedAllocator<float, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>>
        x_pos;
    std::vector<float, utils::AlignedAllocator<float, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>>
        y_pos;
    std::vector<float, utils::AlignedAllocator<float, utils::XoroshiroRNG::REGISTER_BYTE_SIZE>>
        z_pos;
};

}  // namespace benchmarks