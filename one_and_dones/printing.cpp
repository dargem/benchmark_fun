#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

uint32_t string_hash(const std::string& str) { return 0; }

template <size_t N, size_t... IDXS>
uint32_t consteval hash_impl(const char (&arr)[N], std::index_sequence<IDXS...>) {
    // A random probably bad hash, sum value of chars in string multiplied by its (idx + 1)

    uint32_t total{};
    auto summer = [&](size_t idx) -> void { total += static_cast<uint32_t>(arr[idx]) * (idx + 1); };

    (summer(IDXS), ...);
    return total;
}

template <size_t N>
uint32_t consteval string_hash(const char (&arr)[N]) {
    return hash_impl(arr, std::make_index_sequence<N>());
}

int main() {
    char arr[255];
    memcpy(&arr, "Hello World", sizeof("Hello World"));
    std::cout << arr << '\n';   // Decays to a char* pointer
    std::cout << &arr << '\n';  // & returns a char(*) arr[255] which is a pointer to an array, now
                                // the print until null terminator overload isn't called

    std::cout << string_hash("testing") << '\n';

    return 0;
}