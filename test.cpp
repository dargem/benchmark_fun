#include <concepts>

template <auto Tag = [] {}>
class Empty {};

static_assert(!std::same_as<Empty<>, Empty<>>);