#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>

int string_hash(std::string_view) { return 0; }

// A really dumb way to do it
template <size_t N, size_t... IDXS>
int consteval hash_impl(const char (&arr)[N], std::index_sequence<IDXS...>) {
    long long hash{};

    long long p = 31, m = 1e9 + 7;
    long long p_pow = 1;  // Store p ^ i

    auto update = [&](size_t idx) -> void {
        hash = (hash + (arr[idx] - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    };

    (update(IDXS), ...);
    return hash;
}

template <size_t N>
int consteval string_hash(const char (&arr)[N]) {
    // Remove null terminator from hash

    // return hash_impl(arr, std::make_index_sequence<N - 1>());

    long long hash{};

    long long p = 31, m = 1e9 + 7;
    long long p_pow = 1;  // Store p ^ i
    for (size_t idx{}; idx < N - 1; ++idx) {
        hash = (hash + (arr[idx] - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }

    return hash;
}

int main() {
    char arr[255];
    memcpy(&arr, "Hello World", sizeof("Hello World"));
    std::cout << arr << '\n';   // Decays to a char* pointer
    std::cout << &arr << '\n';  // & returns a char(*) arr[255] which is a pointer to an array, now
                                // the print until null terminator overload isn't called

    std::cout << string_hash("testing") << '\n';
    std::string a{"other"};
    std::cout << string_hash(a) << '\n';  // falls back to runtime version

    return 0;
}