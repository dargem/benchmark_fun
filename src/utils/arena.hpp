#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

namespace utils {

template <bool Aligned = true>
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
    template <typename T, size_t N = 1>
        requires std::same_as<std::remove_cvref_t<T>, T> && std::is_trivially_destructible_v<T>
    T* allocate() {
        if constexpr (Aligned) {
            // we need to check we are adding stuff with proper alignment
            // branchless increment to an aligned address, 8 - 13 % 8 gets 3
            current +=
                (alignof(T) - reinterpret_cast<uintptr_t>(current) % alignof(T)) % alignof(T);
            T* ptr = reinterpret_cast<T*>(current);
            current += sizeof(T) * N;
            return ptr;
        } else {
            T* ptr = current;
            current += sizeof(T) * N;
            return ptr;
        }
    }

    /**
     * @brief Allocates space for N objects with sizeof(T)
     *
     * @tparam T the type of the object used in this space
     * @tparam N the number of objects wanted to hold
     * @return T* to the space
     */
    template <typename T>
        requires std::same_as<std::remove_cvref_t<T>, T> && std::is_trivially_destructible_v<T>
    T* allocate(size_t n) {
        if constexpr (Aligned) {
            // we need to check we are adding stuff with proper alignment
            // branchless increment to an aligned address, 8 - 13 % 8 gets 3
            current +=
                (alignof(T) - reinterpret_cast<uintptr_t>(current) % alignof(T)) % alignof(T);
            T* ptr = reinterpret_cast<T*>(current);
            current += sizeof(T) * n;
            return ptr;
        } else {
            T* ptr = current;
            current += sizeof(T) * n;
            return ptr;
        }
    }

    template <typename T, typename... Args>
        requires std::same_as<std::remove_cvref_t<T>, T> && std::is_trivially_destructible_v<T>
    T* emplace(Args&&... args) {
        if constexpr (Aligned) {
            // we need to check we are adding stuff with proper alignment
            // branchless increment to an aligned address, 8 - 13 % 8 gets 3
            current +=
                (alignof(T) - reinterpret_cast<uintptr_t>(current) % alignof(T)) % alignof(T);

            T* ptr = new (current) T(args...);
            current += sizeof(T);

            return ptr;
        } else {
            T* ptr = new (current) T(args...);
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