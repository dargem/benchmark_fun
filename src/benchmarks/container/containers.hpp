#pragma once

#include <list>

#include "benchmarks/benchable.hpp"
#include "utils/sparse_set.hpp"

namespace benchmarks {

enum class Task {
    RANDOM_INSERTION = 0,
    RANDOM_DELETION,
    RANDOM_ACCESS,
    ORDERED_ITERATION,
    UNORDERED_ITERATION,
    BACK_DELETION,
    BACK_INSERTION
};

// T is the task done, C is the container, E is the element
// The containers used are the custom spare set, a vector and a linked list
template <Task T, typename C, typename E>
    requires std::same_as<C, utils::SparseSet<E>> || std::same_as<C, std::vector<E>> ||
             std::same_as<C, std::list<E>>
class ContainerTester : public Benchable {
   public:
    
   private:
};

}  // namespace benchmarks