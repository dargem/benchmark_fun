#include <format>
#include <iostream>

template <unsigned N>
struct Reader {
    friend auto counted_flag(Reader<N>);
};

template <unsigned N>
struct Writer {
    // defines the counted_flag for the unsigned N
    friend auto counted_flag(Reader<N>) {}
};

template <unsigned NextVal, auto Tag = []() {}>
consteval bool check_counted() {
    constexpr bool counted_past_value = requires(Reader<NextVal> r) {
        counted_flag(r);  // will be true if this has a definition
    };

    return counted_past_value;
}

constexpr bool N = check_counted<0>();
Writer<0> a;
constexpr bool O = check_counted<0>();
static_assert(N != O);

constexpr bool P = check_counted<1>();
static_assert(!P);

int main() {
    std::cout << std::format("N is {}, M is {}\n", N, O);
    return 0;
}