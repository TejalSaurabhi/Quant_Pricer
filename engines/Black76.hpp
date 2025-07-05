#pragma once
#include "../core/DiscountCurve.hpp"
#include <cmath>

namespace quant {

// Black-76 model for options on forwards/futures
class Black76 {
public:
  // Calculate option price using Black-76 formula
  // V = D[F*N(d1) - K*N(d2)] for calls
  // V = D[K*N(-d2) - F*N(-d1)] for puts
  static double price(double forwardPrice,   // F
                      double strike,         // K
                      double timeToExpiry,   // T
                      double volatility,     // σ
                      double discountFactor, // D = P(0,T)
                      bool isCall = true);

  // Calculate vega (sensitivity to volatility)
  static double vega(double forwardPrice, double strike, double timeToExpiry,
                     double volatility, double discountFactor);

  // Calculate delta (sensitivity to forward price)
  static double delta(double forwardPrice, double strike, double timeToExpiry,
                      double volatility, double discountFactor,
                      bool isCall = true);

private:
  // Black-Scholes d1 parameter: d1 = [ln(F/K) + 0.5*σ²*T] / (σ*√T)
  static double d1(double F, double K, double T, double sigma);

  // Black-Scholes d2 parameter: d2 = d1 - σ*√T
  static double d2(double F, double K, double T, double sigma);

  // Standard normal cumulative distribution function N(x)
  static double normCDF(double x);

  // Standard normal probability density function φ(x)
  static double normPDF(double x);
};

} // namespace quant