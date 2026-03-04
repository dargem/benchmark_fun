#pragma once

#include <vector>

#include "utils/concepts.hpp"

// because we all love ANOVA don't we
namespace stats {

// used to aggregate some of the treatment stats together
template <utils::Numeric T>
struct TreatmentInfo {
    // clang-format off
    TreatmentInfo(const std::vector<T>& treatment) :
            observations{treatment},
            n{treatment.size()},
            mean{std::accumulate(treatment.begin(), treatment.end(), 0.0,
                                 [](double total, T y) { return total + y; }) / n}
    {}
    // clang-format on

    const std::vector<T>& observations;
    size_t n{};     // number of observations
    double mean{};  // mean observations
};

// Performs a standard ANOVA to check if all treatments have the same mean
// Returns true if null is accepted (all same mean), false if null is rejected (one different
// mean)
template <utils::Numeric T>
bool sameGroupMeans(std::vector<std::vector<T>> treatments) {
    constexpr static double ALPHA = 0.05;
    // H0: u1 = u2 = u3 = etc all same means
    // HA: at least one treatment mean differs

    // First get within group (error deviation) and across group (treatment deviation) variation

    double grand_mean = grand_sum / grand_n;

    // The test statistic is an F-Ratio
};

}  // namespace stats