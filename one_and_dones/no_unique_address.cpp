#include <cstdint>
#include <format>
#include <iostream>

#include "src/utils/xoroshiro_64_star.hpp"

namespace utils {

//  We want to provide access to 3 different types
class HeavyXoroshiroRNG {
   public:
    HeavyXoroshiroRNG(uint32_t seed = 0xcafef00dU) :
            rng(seed),
            raw_uint32_t_buffer(rng.get_batch_uint32()),
            float_buffer(rng.get_batch_floats()) {}

    uint32_t get_uint32() {
        if (raw_idx == XoroshiroRNG::BATCH_SIZE) {
            raw_idx = 0;
            raw_uint32_t_buffer = rng.get_batch_uint32();
        }

        return raw_uint32_t_buffer[raw_idx++];
    }

    int32_t get_int32() {
        // We can share the raw bits used with the uint buffer
        if (raw_idx == XoroshiroRNG::BATCH_SIZE) {
            raw_idx = 0;
            raw_uint32_t_buffer = rng.get_batch_uint32();
        }

        // We just do a bit cast
        return std::bit_cast<int32_t>(raw_uint32_t_buffer[raw_idx++]);
    }

    float get_float() {
        // A distribution of random bits for a float, would not get you a uniform distribution
        // To get a uniform [0, 1) need to set sign 0 and exponent to 127 (0 after offset)
        if (float_idx == XoroshiroRNG::BATCH_SIZE) {
            float_idx = 0;
            float_buffer = rng.get_batch_floats();
        }

        return float_buffer[float_idx++];
    }

   private:
    XoroshiroRNG rng;
    // We need to be aware at a given time whether the buffer is filled with ints or
    // floats. We could have one buffer with a enum member but that adds more branching. So we
    // unfortunately need to use 2 buffers with 2 different idxs to avoid branching on accesses
    std::array<uint32_t, XoroshiroRNG::BATCH_SIZE> raw_uint32_t_buffer;
    std::array<float, XoroshiroRNG::BATCH_SIZE> float_buffer;
    uint8_t raw_idx{};
    uint8_t float_idx{};
};

namespace {
// Don't override this tag
template <auto Tag = [] {}>
class Empty {};

template <typename Term, typename... Set>
concept OneOf = (std::same_as<Term, Set> || ...);
}  // namespace

template <typename... Capabilities>
    requires(OneOf<Capabilities, uint32_t, int32_t, float> && ...)
class BufferedXoroshiroRNG {
   public:
    BufferedXoroshiroRNG(uint32_t seed = 0xcafef00dU) : rng(seed) {}

    uint32_t get_uint32()
        requires OneOf<uint32_t, Capabilities...>
    {
        if (raw_idx == XoroshiroRNG::BATCH_SIZE) {
            raw_idx = 0;
            raw_uint32_t_buffer = rng.get_batch_uint32();
        }

        return raw_uint32_t_buffer[raw_idx++];
    }

    int32_t get_int32()
        requires OneOf<int32_t, Capabilities...>
    {
        // We can share the raw bits used with the uint buffer
        if (raw_idx == XoroshiroRNG::BATCH_SIZE) {
            raw_idx = 0;
            raw_uint32_t_buffer = rng.get_batch_uint32();
        }

        // We just do a bit cast
        return std::bit_cast<int32_t>(raw_uint32_t_buffer[raw_idx++]);
    }

    float get_float()
        requires OneOf<float, Capabilities...>
    {
        // A distribution of random bits for a float, would not get you a uniform distribution
        // To get a uniform [0, 1) need to set sign 0 and exponent to 127 (0 after offset)
        if (float_idx == XoroshiroRNG::BATCH_SIZE) {
            float_idx = 0;
            float_buffer = rng.get_batch_floats();
        }

        return float_buffer[float_idx++];
    }

   public:
    XoroshiroRNG rng;

    // We want to be able to conditionally "disable" a member if its not templated with it
    using UINT_ARR =
        std::conditional_t<OneOf<uint32_t, Capabilities...> || OneOf<int32_t, Capabilities...>,
                           std::array<uint32_t, XoroshiroRNG::BATCH_SIZE>, Empty<>>;
    using UINT_INDEX =
        std::conditional_t<OneOf<uint32_t, Capabilities...> || OneOf<int32_t, Capabilities...>,
                           uint8_t, Empty<>>;
    using FLOAT_ARR = std::conditional_t<OneOf<float, Capabilities...>,
                                         std::array<float, XoroshiroRNG::BATCH_SIZE>, Empty<>>;
    using FLOAT_INDEX = std::conditional_t<OneOf<float, Capabilities...>, uint8_t, Empty<>>;

    [[no_unique_address]] UINT_ARR raw_uint32_t_buffer;
    [[no_unique_address]] FLOAT_ARR float_buffer;
    [[no_unique_address]] UINT_INDEX raw_idx{};
    [[no_unique_address]] FLOAT_INDEX float_idx{};
};

}  // namespace utils

template <typename T>
concept HasFloat = requires(T t) { t.get_float(); };

template <typename T>
concept HasUint32 = requires(T t) { t.get_uint32(); };

template <typename T>
concept HasInt32 = requires(T t) { t.get_int32(); };

static_assert(HasFloat<utils::BufferedXoroshiroRNG<float>> &&
              !HasUint32<utils::BufferedXoroshiroRNG<float>> &&
              !HasInt32<utils::BufferedXoroshiroRNG<float>>);

static_assert(HasFloat<utils::BufferedXoroshiroRNG<float, uint32_t>> &&
              HasUint32<utils::BufferedXoroshiroRNG<float, uint32_t>> &&
              !HasInt32<utils::BufferedXoroshiroRNG<float, uint32_t>>);

int main() {
    std::cout << std::format(
        "uint32 buffered size is {}\n float buffered size is {}\n uint and float buffered size is "
        "{}\n nothing buffered size is {}\n raw XoroshiroRNG size is {}\n",
        sizeof(utils::BufferedXoroshiroRNG<uint32_t>), sizeof(utils::BufferedXoroshiroRNG<float>),
        sizeof(utils::BufferedXoroshiroRNG<uint32_t, float>), sizeof(utils::BufferedXoroshiroRNG<>),
        sizeof(utils::XoroshiroRNG));

    std::cout << offsetof(utils::BufferedXoroshiroRNG<>, rng) << '\n';
    std::cout << offsetof(utils::BufferedXoroshiroRNG<>, raw_uint32_t_buffer) << '\n';
    std::cout << offsetof(utils::BufferedXoroshiroRNG<>, float_buffer) << '\n';
    std::cout << offsetof(utils::BufferedXoroshiroRNG<>, raw_idx) << '\n';
    std::cout << offsetof(utils::BufferedXoroshiroRNG<>, float_idx) << '\n';
}