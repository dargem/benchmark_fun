#include <cstddef>

#include "benchmarks/bench_types.hpp"
#include "benchmarks/benchable.hpp"

namespace benchmarks {

enum class ReservationSize : u_int64_t {
    ZERO_BYTES = 0,
    FIFTEEN_GIGABYTE = 16106127360 /*536870912000*/
};

template <ReservationSize T>  // differentiate static members through reservation size
struct CounterElement {
    // evil mutable static member to count deletions of class
    inline static size_t creationsMade = 0;  // zero initialized
    inline static size_t deletionsMade = 0;  // zero initialized

    size_t value;

    CounterElement(size_t value) { ++creationsMade; }
    CounterElement(const CounterElement&) = default;

    ~CounterElement() { ++deletionsMade; }
};

/**
 * @brief Create a wrapper around a vector
 *
 * @tparam N the initial reserved size of the vector (will resize!)
 */
template <ReservationSize S>
struct VectorWrapper : Benchable {
    std::vector<CounterElement<S>> numbers;

    VectorWrapper() : Benchable(BenchType::VECTOR_RESERVATION, std::string(NAME)) {
        numbers.reserve(static_cast<uint64_t>(S) / sizeof(CounterElement<S>));
    }

    void resetBenchmark() override {
        numbers.clear();
        numbers.shrink_to_fit();
        numbers.reserve(static_cast<uint64_t>(S) / sizeof(CounterElement<S>));
    }

    void runBenchmark(size_t iterations) override {
        for (size_t i{}; i < iterations; ++i) {
            numbers.push_back(CounterElement<S>(i));
        }
    }

    constexpr static std::string_view NAME = [] {
        if constexpr (S == ReservationSize::ZERO_BYTES) {
            return "No Reservation Vector";
        } else {
            return "15 GB reservation vector";
        }
    }();
};

}  // namespace benchmarks