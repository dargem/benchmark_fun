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

    /**
     * @brief Allocates space for N objects with sizeof(T)
     *
     * @tparam T the type of the object used in this space
     * @tparam N the number of objects wanted to hold
     * @return T* to the space
     */
    template <typename T, size_t N>
        requires std::same_as<std::remove_cvref_t<T>, T>
    T* allocate() {
        if constexpr (Aligned) {
            // we need to check we are adding stuff with proper alignment
            // branchless increment to an aligned address, 8 - 13 % 8 gets 3
            current += (alignof(T) - reinterpret_cast<uintptr_t>(current) % alignof(T));
            T* ptr = current;
            current += sizeof(T) * N;
            return ptr;
        } else {
            // just slop it in probably all good!
            T* ptr = current;
            current += sizeof(T) * N;
            return ptr;
        }
    }

    template <typename T, typename... Ts>
    T* create(Ts... args) {
        current += (alignof(T) - reinterpret_cast<uintptr_t>(current) % alignof(T));
        T* ptr = new (current) T(args...);
        current += sizeof(T);
    }

    void reset() { current = begin; }

   private:
    static constexpr size_t CAPACITY = 1000000;  // Capacity in bytes

    std::byte* begin;
    std::byte* end;
    std::byte* current;  // next free byte
};

}  // namespace utils