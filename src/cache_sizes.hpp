#pragma once
#include <cstddef>

namespace cache {

constexpr size_t L1D_SIZE{32 * 1024};        // 32 kilobytes
constexpr size_t L1I_SIZE{32 * 1024};        // 32 kilobytes
constexpr size_t L2_SIZE{1 * 1024 * 1024};   // 1 megabytes
constexpr size_t L3_SIZE{16 * 1024 * 1024};  // 16 megabytes

}  // namespace cache