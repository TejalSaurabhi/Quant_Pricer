#include "Bond.hpp"
#include <cmath>

namespace quant {

Bond::Bond(double face, double cpnRate, int couponPerYear,
           double maturityYears) {
  // Generate cash flows using bulletSchedule
  cfs_ = bulletSchedule(face, cpnRate, couponPerYear, maturityYears);
}

double Bond::price(const DiscountCurve &curve) const {
  double price = 0.0;

  for (const auto &cf : cfs_) {
    double df = curve.df(cf.time);
    price += cf.amount * df;
  }

  return price;
}

double Bond::yieldFromPrice(double cleanPrice, Compounding m,
                            const YieldSolver &solver) const {
  // Use the new solver interface
  return solver.solve(*this, cleanPrice, m);
}

double Bond::dv01(const DiscountCurve &curve, Compounding m) const {
  double yield = extractYield(curve, m);
  return Sensitivity::dv01(cfs_, yield, m);
}

double Bond::modDuration(const DiscountCurve &curve, Compounding m) const {
  double yield = extractYield(curve, m);
  return Sensitivity::modifiedDuration(cfs_, yield, m);
}

double Bond::convexity(const DiscountCurve &curve, Compounding m) const {
  double yield = extractYield(curve, m);
  return Sensitivity::convexity(cfs_, yield, m);
}

double Bond::extractYield(const DiscountCurve &curve, Compounding m) const {
  // Simplified approach: extract yield from the average time to maturity

  if (cfs_.empty())
    return 0.05; // Default fallback

  // Use the time of the last cash flow as proxy for maturity
  double maturityTime = cfs_.back().time;
  double df = curve.df(maturityTime);

  if (df <= 0.0)
    return 0.05; // Fallback for invalid discount factor

  // Convert discount factor back to yield: df = (1 + y/m)^(-m*T)
  // Therefore: y = m * ((1/df)^(1/(m*T)) - 1)

  if (m == Compounding::Continuous) {
    // For continuous: df = e^(-y*T), so y = -ln(df)/T
    return -std::log(df) / maturityTime;
  } else {
    double compoundingFreq = static_cast<double>(m);
    double yieldFactor =
        std::pow(1.0 / df, 1.0 / (compoundingFreq * maturityTime));
    return compoundingFreq * (yieldFactor - 1.0);
  }
}

} // namespace quant