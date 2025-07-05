#pragma once
#include "../core/DiscountCurve.hpp"

namespace quant {

// Forward declaration to avoid circular dependency
class Bond;

class YieldSolver {
public:
  double solve(const Bond &b, double targetPrice, Compounding m,
               double y0 = 0.05) const;

private:
  // Helper: Calculate price difference f(y) = price(y) - targetPrice
  double priceDifference(const Bond &b, double yield, double targetPrice,
                         Compounding m) const;

  // Helper: Calculate derivative f'(y) for Newton-Raphson
  double priceDifferenceDerivative(const Bond &b, double yield,
                                   Compounding m) const;

  // Bisection method for initial 10 iterations
  double bisectionPhase(const Bond &b, double targetPrice, Compounding m) const;

  // Newton-Raphson method after bisection
  double newtonRaphsonPhase(const Bond &b, double targetPrice, Compounding m,
                            double y0) const;
};

} // namespace quant