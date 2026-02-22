#pragma once

#include <algorithm>
#include <cstddef>
#include <src/benchmarks/branch_prediction/branch_prediction_unsorted.hpp>
#include <string_view>
#include <vector>

namespace benchmarks {

class BranchPredictionSorted : public BranchPredictionUnsorted {
   public:
    BranchPredictionSorted(size_t listSize) : BranchPredictionUnsorted(listSize, NAME) {
        // On construction just sort the numbers of parent lazy way to do this
        std::vector<int>& numbers{BranchPredictionUnsorted::getInternalNumbersVector()};
        std::sort(numbers.begin(), numbers.end());
    }

    void resetBenchmark() {
        // call base reset benchmark getting new random numbers
        BranchPredictionUnsorted::resetBenchmark();
        std::vector<int>& numbers{BranchPredictionUnsorted::getInternalNumbersVector()};
        std::sort(numbers.begin(), numbers.end());
    }

   private:
    static constexpr std::string_view NAME{"Branch Prediction Sorted Version"};
};

}  // namespace benchmarks