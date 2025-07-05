#pragma once
#include "../core/CashFlow.hpp"
#include "../core/DiscountCurve.hpp"
#include "../engines/Sensitivity.hpp"
#include "../engines/YieldSolver.hpp"
#include <vector>

// For bulletSchedule function
using quant::bulletSchedule;

namespace quant {

class Bond {
public:
  Bond(double face, double cpnRate, int couponPerYear, double maturityYears);

  double price(const DiscountCurve &curve) const;
  double yieldFromPrice(double cleanPrice, Compounding m,
                        const YieldSolver &solver) const;

  // Analytics
  double dv01(const DiscountCurve &curve, Compounding m) const;
  double modDuration(const DiscountCurve &curve, Compounding m) const;
  double convexity(const DiscountCurve &curve, Compounding m) const;

private:
  std::vector<CashFlow> cfs_;

  // Helper to extract yield from discount curve (simplified assumption)
  double extractYield(const DiscountCurve &curve, Compounding m) const;
};

} // namespace quant