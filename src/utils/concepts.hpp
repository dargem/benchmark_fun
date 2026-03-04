#pragma once

#include <concepts>

namespace utils {

// Use to constrain template parameters to just arithmetic types
// e.g. doubles, integers, char even since you can do math on them
// would give a clean error message on something like a string_view though
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

}  // namespace utils