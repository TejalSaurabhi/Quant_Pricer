#pragma once
#include "../core/CashFlow.hpp"
#include "../core/DiscountCurve.hpp"
#include <vector>

namespace quant {

// Analytic sensitivity calculations for bonds
class Sensitivity {
public:
  // Calculate price using analytic formula
  static double price(const std::vector<CashFlow> &cashFlows, double yield,
                      Compounding compounding);

  // Calculate first derivative of price with respect to yield: ∂P/∂y
  static double priceDelta(const std::vector<CashFlow> &cashFlows, double yield,
                           Compounding compounding);

  // Calculate second derivative of price with respect to yield: ∂²P/∂y²
  static double priceGamma(const std::vector<CashFlow> &cashFlows, double yield,
                           Compounding compounding);

  // Modified duration: -(1/P) * (∂P/∂y)
  static double modifiedDuration(const std::vector<CashFlow> &cashFlows,
                                 double yield, Compounding compounding);

  // DV01: Dollar value of 1 basis point
  static double dv01(const std::vector<CashFlow> &cashFlows, double yield,
                     Compounding compounding);

  // Convexity: (1/P) * (∂²P/∂y²)
  static double convexity(const std::vector<CashFlow> &cashFlows, double yield,
                          Compounding compounding);

private:
  // Helper: Calculate discount factor for a given time and yield
  static double discountFactor(double time, double yield,
                               Compounding compounding);

  // Helper: Calculate first derivative of discount factor
  static double discountFactorDelta(double time, double yield,
                                    Compounding compounding);

  // Helper: Calculate second derivative of discount factor
  static double discountFactorGamma(double time, double yield,
                                    Compounding compounding);
};

} // namespace quant