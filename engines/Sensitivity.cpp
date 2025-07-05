#include "Sensitivity.hpp"
#include <cmath>

namespace quant {

double Sensitivity::price(const std::vector<CashFlow> &cashFlows, double yield,
                          Compounding compounding) {
  double P = 0.0;

  for (const auto &cf : cashFlows) {
    double df = discountFactor(cf.time, yield, compounding);
    P += cf.amount * df;
  }

  return P;
}

double Sensitivity::priceDelta(const std::vector<CashFlow> &cashFlows,
                               double yield, Compounding compounding) {
  // ∂P/∂y = -∑ CFᵢ * tᵢ * (1 + y/m)^(-mtᵢ-1) for discrete compounding
  // ∂P/∂y = -∑ CFᵢ * tᵢ * e^(-ytᵢ) for continuous compounding

  double dP_dy = 0.0;

  for (const auto &cf : cashFlows) {
    double delta_df = discountFactorDelta(cf.time, yield, compounding);
    dP_dy += cf.amount * delta_df;
  }

  return dP_dy;
}

double Sensitivity::priceGamma(const std::vector<CashFlow> &cashFlows,
                               double yield, Compounding compounding) {
  // ∂²P/∂y² - second derivative

  double d2P_dy2 = 0.0;

  for (const auto &cf : cashFlows) {
    double gamma_df = discountFactorGamma(cf.time, yield, compounding);
    d2P_dy2 += cf.amount * gamma_df;
  }

  return d2P_dy2;
}

double Sensitivity::modifiedDuration(const std::vector<CashFlow> &cashFlows,
                                     double yield, Compounding compounding) {
  // Modified Duration = -(1/P) * (∂P/∂y)

  double P = price(cashFlows, yield, compounding);
  double dP_dy = priceDelta(cashFlows, yield, compounding);

  if (P == 0.0)
    return 0.0;

  return -dP_dy / P;
}

double Sensitivity::dv01(const std::vector<CashFlow> &cashFlows, double yield,
                         Compounding compounding) {
  // DV01 = Dollar value of 1 basis point = -(∂P/∂y) * 0.0001

  double dP_dy = priceDelta(cashFlows, yield, compounding);
  return -dP_dy * 0.0001; // 1 basis point = 0.0001
}

double Sensitivity::convexity(const std::vector<CashFlow> &cashFlows,
                              double yield, Compounding compounding) {
  // Convexity = (1/P) * (∂²P/∂y²)

  double P = price(cashFlows, yield, compounding);
  double d2P_dy2 = priceGamma(cashFlows, yield, compounding);

  if (P == 0.0)
    return 0.0;

  return d2P_dy2 / P;
}

// Helper functions

double Sensitivity::discountFactor(double time, double yield,
                                   Compounding compounding) {
  if (compounding == Compounding::Continuous) {
    return std::exp(-yield * time);
  } else {
    double m = static_cast<double>(compounding);
    return std::pow(1.0 + yield / m, -m * time);
  }
}

double Sensitivity::discountFactorDelta(double time, double yield,
                                        Compounding compounding) {
  // First derivative of discount factor with respect to yield

  if (compounding == Compounding::Continuous) {
    // d/dy[e^(-yt)] = -t * e^(-yt)
    return -time * std::exp(-yield * time);
  } else {
    // d/dy[(1 + y/m)^(-mt)] = -t * (1 + y/m)^(-mt-1)
    double m = static_cast<double>(compounding);
    double base = 1.0 + yield / m;
    return -time * std::pow(base, -m * time - 1.0);
  }
}

double Sensitivity::discountFactorGamma(double time, double yield,
                                        Compounding compounding) {
  // Second derivative of discount factor with respect to yield

  if (compounding == Compounding::Continuous) {
    // d²/dy²[e^(-yt)] = t² * e^(-yt)
    return time * time * std::exp(-yield * time);
  } else {
    // d²/dy²[(1 + y/m)^(-mt)] = (t² + t/m) * (1 + y/m)^(-mt-2)
    // More algebraically precise form
    double m = static_cast<double>(compounding);
    double base = 1.0 + yield / m;
    return (time * time + time / m) * std::pow(base, -m * time - 2.0);
  }
}

} // namespace quant