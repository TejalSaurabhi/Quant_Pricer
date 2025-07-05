#pragma once
#include <vector>

namespace quant {

struct CashFlow {
  double time;
  double amount;
};

// Generate bullet bond cash flow schedule
[[nodiscard]] std::vector<CashFlow> bulletSchedule(double face, double cpnRate,
                                                   int couponPerYear = 2,
                                                   double maturityYears = 1.0);

} // namespace quant