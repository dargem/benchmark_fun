#pragma once

#include <boost/math/distributions/fisher_f.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <numeric>
#include <vector>

#include "utils/concepts.hpp"

namespace stats {

struct ConfidenceInterval {
    double pointEstimate{};  // the point estimate (centre) of the interval
    double lowBound{};       // lower bounds of the interval
    double highBound{};      // high bounds of the interval
    double confidence{};     // confidence in the interval
};

template <utils::Numeric T>
ConfidenceInterval generateConfidenceInterval(std::vector<T> samples) {
    // use a 95% confidence interval so use 5% alpha (5% chance of a type 1 error)
    constexpr static double ALPHA{0.05};
    size_t n = samples.size();  // number of samples

    ConfidenceInterval confidenceInterval{};
    // confidence interval is x_bar ± t * (s / sqrt(n))
    // first try to find x_bar aka point estimate
    T sum = std::accumulate(samples.begin(), samples.end(), 0,
                            [](T sum, T sample) { return sum + sample; });
    double sampleMean = static_cast<double>(sum) / n;
    confidenceInterval.pointEstimate = sampleMean;

    // then find the used t value based on us wanting a (1-ALPHA)*100% confidence interval
    boost::math::students_t dist(n - 1);  // n - 1 degrees of freedom
    const double tScore{boost::math::quantile(dist, 1.0 - (ALPHA / 2.0))};
    confidenceInterval.confidence = 1.0 - ALPHA;

    // then find sample deviation, first finding sum of cubed deviations
    T cubedDeviations =
        std::accumulate(samples.begin(), samples.end(), 0, [sampleMean](T currentSum, T sample) {
            return currentSum + std::pow(sample - sampleMean, 2);
        });
    // then divide it by n-1 samples and root it to get an unbiased estimator of standard deviation
    double sampleStandardDeviation = std::sqrt(static_cast<double>(cubedDeviations) / (n - 1));

    // the magnitude of the margin in a direction from the point estimate is t * (s / sqrt(n))
    const double margin{tScore * sampleStandardDeviation / std::sqrt(n)};

    confidenceInterval.highBound = confidenceInterval.pointEstimate + margin;
    confidenceInterval.lowBound = confidenceInterval.pointEstimate - margin;

    return confidenceInterval;
};

}  // namespace stats