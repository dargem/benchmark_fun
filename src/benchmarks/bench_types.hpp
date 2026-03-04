#pragma once

namespace benchmarks {

// benchmark types
enum class BenchType {
    RANDOM_NUMBER_GENERATION,
    BRANCH_PREDICTION,
    ATTRIBUTE_BRANCH_PREDICTION,
    STRUCTURE_ITERATION,
    VECTOR_BOOL_CHAR_RANDOM_ACCESS,
    STRUCTURE_LAYOUT
};

}  // namespace benchmarks