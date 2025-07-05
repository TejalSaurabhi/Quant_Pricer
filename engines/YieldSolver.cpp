#include "YieldSolver.hpp"
#include "../engines/Sensitivity.hpp"
#include "../instruments/Bond.hpp"
#include <cmath>
#include <stdexcept>

namespace quant {

double YieldSolver::solve(const Bond &b, double targetPrice, Compounding m,
                          double y0) const {

  // Phase 1: Bisection for exactly 10 iterations on [0, 1]
  double yieldBisection = bisectionPhase(b, targetPrice, m);

  // Phase 2: Newton-Raphson starting from bisection result
  double yieldFinal = newtonRaphsonPhase(b, targetPrice, m, yieldBisection);

  return yieldFinal;
}

double YieldSolver::bisectionPhase(const Bond &b, double targetPrice,
                                   Compounding m) const {
  double a = 0.0; // Lower bound
  double b_upper =
      1.0; // Upper bound (avoiding variable name conflict with bond 'b')

  // Check if root is bracketed
  double fa = priceDifference(b, a, targetPrice, m);
  double fb = priceDifference(b, b_upper, targetPrice, m);

  // If not bracketed, expand bounds (safety measure)
  if (fa * fb > 0) {
    // Try expanding upper bound
    b_upper = 2.0;
    fb = priceDifference(b, b_upper, targetPrice, m);

    if (fa * fb > 0) {
      // Still not bracketed, throw error for better diagnostics
      throw std::runtime_error(
          "YieldSolver: unable to bracket root in range [0, 2]");
    }
  }

  // Perform exactly 10 bisection iterations
  for (int i = 0; i < 10; ++i) {
    double c = (a + b_upper) / 2.0;
    double fc = priceDifference(b, c, targetPrice, m);

    if (fa * fc < 0) {
      b_upper = c;
      fb = fc;
    } else {
      a = c;
      fa = fc;
    }
  }

  return (a + b_upper) / 2.0;
}

double YieldSolver::newtonRaphsonPhase(const Bond &b, double targetPrice,
                                       Compounding m, double y0) const {
  double y = y0;
  const double tolerance = 1e-12;
  const int maxIterations = 100;

  for (int i = 0; i < maxIterations; ++i) {
    double priceError = priceDifference(b, y, targetPrice, m);

    // Check convergence: |ΔP| < 1e-12
    if (std::abs(priceError) < tolerance) {
      return y;
    }

    double priceDerivative = priceDifferenceDerivative(b, y, m);

    // Avoid division by zero
    if (std::abs(priceDerivative) < 1e-15) {
      break; // Cannot continue with Newton-Raphson
    }

    // Newton-Raphson update: y_{n+1} = y_n - f(y_n)/f'(y_n)
    y = y - priceError / priceDerivative;

    // Keep yield in reasonable bounds
    if (y < 0.0)
      y = 0.001;
    if (y > 2.0)
      y = 2.0;
  }

  return y;
}

double YieldSolver::priceDifference(const Bond &b, double yield,
                                    double targetPrice, Compounding m) const {
  // Create a flat discount curve with the given yield
  DiscountCurve curve(yield, m, DayCount::ACT_365F);

  // Calculate bond price using this curve
  double bondPrice = b.price(curve);

  // Return f(y) = price(y) - targetPrice
  return bondPrice - targetPrice;
}

double YieldSolver::priceDifferenceDerivative(const Bond &b, double yield,
                                              Compounding m) const {
  // Use numerical differentiation with adaptive step size for stability
  const double h = std::max(1e-8, 1e-6 * std::abs(yield));

  double f_plus = priceDifference(
      b, yield + h, 0.0, m); // targetPrice=0 since we only need difference
  double f_minus = priceDifference(b, yield - h, 0.0, m);

  // Central difference: f'(y) ≈ (f(y+h) - f(y-h)) / (2h)
  return (f_plus - f_minus) / (2.0 * h);
}

} // namespace quant