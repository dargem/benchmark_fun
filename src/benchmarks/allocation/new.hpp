#pragma once

#include <cstddef>
#include <vector>

namespace benchmarks {

class NewWrapper {
   public:
    template <typename T, size_t N = 1>
    T* allocate() {
        if constexpr (N == 1) {
            return new T();
        } else {
            return new T[N];
        }
    }

    template <typename T>
    T* allocate(size_t N) {
        return new T[N];
    }

    template <typename T>
    void deallocate(T* ptr) {
        delete ptr;
    }

    template <typename T>
    void deallocate(const std::vector<T*>& ptrs) {
        for (T* ptr : ptrs) {
            delete ptr;
        }
    }

   private:
};

}  // namespace benchmarks