#pragma once

#include <cstddef>
#include <vector>

namespace benchmarks {

class NewWrapper {
   public:
    template <typename T, size_t N = 1>
    T* allocate() {
        if constexpr (N == 1) {
            T* ptr = new T();
        } else {
            T* ptr = new T[N];
        }
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