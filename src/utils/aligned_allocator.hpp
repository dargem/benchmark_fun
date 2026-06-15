#pragma once

#include <stdlib.h>

#include <new>

namespace utils {

template <typename E, std::size_t BYTE_ALIGN = std::hardware_destructive_interference_size>
class AlignedAllocator {
    static_assert(alignof(E) <= BYTE_ALIGN, "Aligned to a value smaller than element");

   public:
    using value_type = E;
    static constexpr std::align_val_t ALIGNMENT{BYTE_ALIGN};

    AlignedAllocator() noexcept {};

    template <typename T>
    AlignedAllocator(const AlignedAllocator<T, BYTE_ALIGN>&) noexcept {}

    template <typename T>
    inline bool operator==(const AlignedAllocator<T, BYTE_ALIGN>& other) const noexcept {
        return true;
    }

    template <typename T>
    inline bool operator!=(const AlignedAllocator<T, BYTE_ALIGN>& other) const noexcept {
        return false;
    }

    inline value_type* allocate(const std::size_t n) const {
        value_type* ptr;
        auto ret = posix_memalign((void**)&ptr, BYTE_ALIGN, sizeof(E) * n);
        if (ret != 0) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    // We need to implement a rebind as our Allocator isn't only templated by type
    // So we need to explicitly tell it how to rebind the allocator to a different type
    template <typename U>
    struct rebind {
        using other = AlignedAllocator<U, BYTE_ALIGN>;
    };

    inline void deallocate(value_type* ptr, std::size_t n) const noexcept { free(ptr); }
};

}  // namespace utils