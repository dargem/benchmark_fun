#pragma once

#include <src/benchmarks/branch_prediction_unsorted.hpp>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <string_view>

namespace benchmarks {
  
class BranchPredictionSorted : public BranchPredictionUnsorted {
public:
    BranchPredictionSorted(size_t listSize) 
        : BranchPredictionUnsorted(listSize, NAME)
    {
        // On construction just sort the numbers of parent lazy way to do this
        std::vector<double>& numbers{BranchPredictionUnsorted::getInternalNumbersVector()};
        std::sort(numbers.begin(), numbers.end());
    }

    void resetBenchmark() {
        // call base reset benchmark getting new random numbers
        BranchPredictionUnsorted::resetBenchmark();
        std::vector<double>& numbers{BranchPredictionUnsorted::getInternalNumbersVector()};
        std::sort(numbers.begin(), numbers.end());
    }

private:
    static constexpr std::string_view NAME{"Branch Prediction Sorted Version"};
};

}