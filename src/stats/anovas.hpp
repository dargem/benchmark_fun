#pragma once

#include <cassert>
#include <cmath>
#include <numeric>
#include <ranges>
#include <vector>

#include "boost/math/distributions/fisher_f.hpp"
#include "utils/concepts.hpp"

// because we all love ANOVA don't we
namespace stats {

// Performs a standard one way ANOVA to check if all treatments have the same mean. Returns true if
// null is accepted (all same mean), false if null is rejected (one different mean)
template <utils::Numeric T>
bool sameGroupMeans(const std::vector<std::vector<T>>& treatments) {
    assert(treatments.size() >= 3);  // should compare 3 or more groups
    constexpr static double ALPHA = 0.05;
    auto observations = std::views::all(treatments) | std::views::join;

    // H0: u1 = u2 = u3 = etc all same means
    // HA: at least one treatment mean differs

    // First get within group (error deviation) and across group (treatment deviation) mean squares.
    // Sum of Squares Within is = for each observation in each group, find its deviation from its
    // own group mean. Then sum it all together. Its degrees of freedom is n. observations -
    // n.groups. To get mean square of within divide SSW by its degrees of freedom.
    double SSW{};
    for (const std::vector<T>& treatment : treatments) {
        assert(!treatment.empty());
        double treatmentMean =
            std::accumulate(treatment.begin(), treatment.end(), 0.0,
                            [](double rollingSum, T val) { return rollingSum + val; }) /
            treatment.size();

        for (T observation : treatment) {
            SSW += std::pow(observation - treatmentMean, 2);
        }
    }
    size_t withinDF = observations.size() - treatments.size();
    assert(withinDF > 0);
    double MSW = SSW / withinDF;  // Now have the mean square within

    // To get the mean squares across/between, for each group sum the deviation of that groups mean
    // from the grand mean. It has degrees of freedom of k - 1, where k is number of groups.
    // MSB (mean square between) = SSB / DF.

    double grandMean =
        std::accumulate(observations.begin(), observations.end(), 0.0,
                        [](double sum, T observation) { return sum + observation; }) /
        observations.size();

    double SSB{};
    for (const std::vector<T>& treatment : treatments) {
        double treatmentMean =
            std::accumulate(treatment.begin(), treatment.end(), 0.0,
                            [](double sum, T observation) { return sum + observation; }) /
            treatment.size();
        SSB += treatment.size() * std::pow(treatmentMean - grandMean, 2);
    }
    size_t betweenDF = treatments.size() - 1;
    double MSB = SSB / betweenDF;

    // The test statistic is an F statistic, where F = MSB / MSW. If F > 1, this means more error is
    // attributed to variation across groups than within groups.
    double F = MSB / MSW;
    boost::math::fisher_f_distribution f_dist(betweenDF, withinDF);
    // One tailed cutoff
    const double criticalPoint = boost::math::quantile(f_dist, (1.0 - ALPHA));

    // If F is smaller or equal than the critical point the null hypothesis is accepted that they
    // all have equal means. So return true.
    return F <= criticalPoint;
}

}  // namespace stats