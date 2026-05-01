#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace utils {

template <bool Aligned>
class Arena {
   public:
    Arena() : begin(new std::byte[CAPACITY]), end(begin + CAPACITY), current(begin) {}

    // delete copy construction / assignment
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // delete move construction / assignment
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    ~Arena() { delete[] begin; }

    template <typename T>
        requires std::same_as<std::remove_cvref_t<T>, T>
    T* allocate() {
        if constexpr (Aligned) {
            // we need to check we are adding stuff with proper alignment
            // branchless increment to an aligned address
            current += (reinterpret_cast<uintptr_t>(current) % alignof(T));
            T* ptr = current;
            current += sizeof(T);
            return ptr;
        } else {
            T* ptr = current;
            current += sizeof(T);
            return ptr;
        }
    }

    void reset() { current = begin; }

   private:
    static constexpr size_t CAPACITY = 1000000;  // Capacity in bytes

    std::byte* begin;
    std::byte* end;
    std::byte* current;  // next free byte
};

}  // namespace utils