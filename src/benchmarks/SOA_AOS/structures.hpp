#pragma once

#include <vector>

#include "src/benchmarks/bench_types.hpp"
#include "src/benchmarks/benchable.hpp"

namespace benchmarks {

namespace {
// Entity types
struct Entity {
    double attack{1.0};
    double defense{1.0};
    double health{1.0};
    double x{};
    double y{};
    double z{};
};

}  // namespace

class AOS : public Benchable {
   public:
    AOS(size_t numEntities) :
            Benchable(BenchType::STRUCTURE_LAYOUT, "Array of Structures Iteration"),
            entities(numEntities, Entity{}) {}

   private:
    std::vector<Entity> entities;  // Entity types
};

}  // namespace benchmarks