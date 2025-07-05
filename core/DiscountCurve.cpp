#include "DiscountCurve.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// Make sure we use the same span definition
#if __cpp_lib_span >= 202002L
#define QUANT_SPAN std::span
#else
#define QUANT_SPAN span_view
#endif

namespace quant {

// Flat constructor
DiscountCurve::DiscountCurve(double flatYield, Compounding cmp, DayCount dc)
    : y_(flatYield), m_(cmp), dc_(dc) {
  // Validate inputs
  if (std::isnan(flatYield) || std::isinf(flatYield)) {
    throw std::invalid_argument("Invalid yield: must be finite");
  }
  // Note: Negative yields are allowed for certain market conditions
  // Empty boot_ vector indicates flat curve
}

// Boot-strapped constructor
DiscountCurve::DiscountCurve(QUANT_SPAN<const ZeroQuote> quotes)
    : y_(0.0), m_(Compounding::Continuous), dc_(DayCount::ACT_365F) {
  if (quotes.empty()) {
    throw std::invalid_argument(
        "Cannot create bootstrapped curve with empty quotes");
  }

  // Copy quotes to internal storage
  boot_.assign(quotes.begin(), quotes.end());

  // Validate quotes
  for (const auto &quote : boot_) {
    if (quote.time <= 0.0 || std::isnan(quote.time) || std::isinf(quote.time)) {
      throw std::invalid_argument(
          "Invalid quote time: must be positive and finite");
    }
    if (quote.df <= 0.0 || std::isnan(quote.df) || std::isinf(quote.df)) {
      throw std::invalid_argument(
          "Invalid discount factor: must be positive and finite");
    }
  }

  // Sort by time for interpolation
  std::sort(
      boot_.begin(), boot_.end(),
      [](const ZeroQuote &a, const ZeroQuote &b) { return a.time < b.time; });
}

double DiscountCurve::df(double t) const {
  // Validate input
  if (std::isnan(t) || std::isinf(t)) {
    throw std::invalid_argument("Time must be finite");
  }

  if (!boot_.empty()) {
    // Bootstrapped curve - interpolate discount factors
    if (t <= 0.0)
      return 1.0;

    // Find surrounding points
    auto it = std::lower_bound(
        boot_.begin(), boot_.end(), ZeroQuote{t, 0.0},
        [](const ZeroQuote &a, const ZeroQuote &b) { return a.time < b.time; });

    if (it == boot_.begin()) {
      // Before first point - flat extrapolation
      return boot_[0].df;
    } else if (it == boot_.end()) {
      // After last point - flat extrapolation
      return boot_.back().df;
    } else {
      // Log-linear interpolation of discount factors (preserves positivity)
      auto prev = it - 1;
      double t0 = prev->time;
      double t1 = it->time;
      double df0 = prev->df;
      double df1 = it->df;

      // Ensure positive discount factors for log interpolation
      if (df0 <= 0.0 || df1 <= 0.0) {
        // Fallback to linear if any DF is non-positive
        if (t1 == t0) {
          return df0;
        }
        double weight = (t - t0) / (t1 - t0);
        return df0 + weight * (df1 - df0);
      }

      // Log-linear: ln(df) interpolated linearly
      if (t1 == t0) {
        // Degenerate case: same time points
        return df0;
      }
      double weight = (t - t0) / (t1 - t0);
      double logDF = std::log(df0) + weight * (std::log(df1) - std::log(df0));
      return std::exp(logDF);
    }
  } else {
    // Flat curve - use analytical formula
    if (t <= 0.0)
      return 1.0;

    if (m_ == Compounding::Continuous) {
      // P(0,t) = e^(-y*t)
      return std::exp(-y_ * t);
    } else {
      // P(0,t) = (1 + y/m)^(-m*t)
      double m = static_cast<double>(m_);
      if (m == 0.0) {
        // Fallback to continuous (should not happen due to earlier if)
        return std::exp(-y_ * t);
      }
      return std::pow(1.0 + y_ / m, -m * t);
    }
  }
}

double DiscountCurve::fwdBondPrice(double t) const {
  // Forward bond price = 1 / discount factor
  double discount = df(t);
  return (discount > 0.0) ? 1.0 / discount : 0.0;
}

} // namespace quant