#pragma once

#include <src/benchmarks/branch_prediction_unsorted.hpp>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace benchmarks {
  
class BranchPredictionSorted : public BranchPredictionUnsorted {
public:
    BranchPredictionSorted(size_t listSize) 
        : BranchPredictionUnsorted(listSize)
    {
        // On construction just sort the numbers of parent lazy way to do this
        std::vector<double>& numbers{BranchPredictionUnsorted::getInternalNumbersVector()};
        std::sort(numbers.begin(), numbers.end());
    }
};

}