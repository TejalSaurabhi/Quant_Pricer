#include "Black76.hpp"
#include <cmath>

// Use std::numbers::pi when available, fallback for compatibility
#if __cpp_lib_math_constants >= 201907L
#include <numbers>
#define QUANT_PI std::numbers::pi
#else
#define QUANT_PI 3.14159265358979323846
#endif

namespace quant {

double Black76::price(double forwardPrice, double strike, double timeToExpiry,
                      double volatility, double discountFactor, bool isCall) {

  if (timeToExpiry <= 0.0 || volatility <= 0.0) {
    // Intrinsic value for expired/zero-vol options
    if (isCall) {
      return discountFactor * std::max(forwardPrice - strike, 0.0);
    } else {
      return discountFactor * std::max(strike - forwardPrice, 0.0);
    }
  }

  double d1_val = d1(forwardPrice, strike, timeToExpiry, volatility);
  double d2_val = d2(forwardPrice, strike, timeToExpiry, volatility);

  if (isCall) {
    // Call: V = D[F*N(d1) - K*N(d2)]
    return discountFactor *
           (forwardPrice * normCDF(d1_val) - strike * normCDF(d2_val));
  } else {
    // Put: V = D[K*N(-d2) - F*N(-d1)]
    return discountFactor *
           (strike * normCDF(-d2_val) - forwardPrice * normCDF(-d1_val));
  }
}

double Black76::vega(double forwardPrice, double strike, double timeToExpiry,
                     double volatility, double discountFactor) {

  if (timeToExpiry <= 0.0 || volatility <= 0.0) {
    return 0.0;
  }

  double d1_val = d1(forwardPrice, strike, timeToExpiry, volatility);

  // Vega = D * F * φ(d1) * √T
  return discountFactor * forwardPrice * normPDF(d1_val) *
         std::sqrt(timeToExpiry);
}

double Black76::delta(double forwardPrice, double strike, double timeToExpiry,
                      double volatility, double discountFactor, bool isCall) {

  if (timeToExpiry <= 0.0 || volatility <= 0.0) {
    // Delta for expired options
    if (isCall) {
      return discountFactor * (forwardPrice > strike ? 1.0 : 0.0);
    } else {
      return discountFactor * (forwardPrice < strike ? -1.0 : 0.0);
    }
  }

  double d1_val = d1(forwardPrice, strike, timeToExpiry, volatility);

  if (isCall) {
    // Call delta = D * N(d1)
    return discountFactor * normCDF(d1_val);
  } else {
    // Put delta = -D * N(-d1)
    return -discountFactor * normCDF(-d1_val);
  }
}

// Private helper functions

double Black76::d1(double F, double K, double T, double sigma) {
  // d1 = [ln(F/K) + 0.5*σ²*T] / (σ*√T)
  return (std::log(F / K) + 0.5 * sigma * sigma * T) / (sigma * std::sqrt(T));
}

double Black76::d2(double F, double K, double T, double sigma) {
  // d2 = d1 - σ*√T
  return d1(F, K, T, sigma) - sigma * std::sqrt(T);
}

double Black76::normCDF(double x) {
  // Approximation of standard normal CDF using error function
  return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
}

double Black76::normPDF(double x) {
  // Standard normal PDF: φ(x) = (1/√(2π)) * e^(-x²/2)
  return (1.0 / std::sqrt(2.0 * QUANT_PI)) * std::exp(-0.5 * x * x);
}

} // namespace quant