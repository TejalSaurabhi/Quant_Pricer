#include "CashFlow.hpp"
#include <cmath>
#include <stdexcept>

namespace quant {

std::vector<CashFlow> bulletSchedule(double face, double cpnRate,
                                     int couponPerYear, double maturityYears) {
  std::vector<CashFlow> cashFlows;

  // Enhanced input validation
  if (std::isnan(face) || std::isinf(face)) {
    throw std::invalid_argument("Face value must be finite");
  }
  if (std::isnan(cpnRate) || std::isinf(cpnRate)) {
    throw std::invalid_argument("Coupon rate must be finite");
  }
  if (std::isnan(maturityYears) || std::isinf(maturityYears)) {
    throw std::invalid_argument("Maturity must be finite");
  }

  if (maturityYears <= 0.0) {
    throw std::invalid_argument("Maturity must be positive");
  }
  if (couponPerYear <= 0) {
    throw std::invalid_argument("Coupon frequency must be positive");
  }
  if (face <= 0.0) {
    throw std::invalid_argument("Face value must be positive");
  }
  // Note: Negative coupon rates are allowed (some market instruments)

  // Calculate coupon amount per payment
  double couponAmount = (cpnRate * face) / couponPerYear;

  // Generate coupon payments
  double timeStep = 1.0 / couponPerYear;
  int totalPayments = std::lround(maturityYears * couponPerYear);

  for (int i = 1; i <= totalPayments; ++i) {
    double time;
    if (i == totalPayments) {
      // Ensure final payment is exactly at maturity
      time = maturityYears;
    } else {
      // Regular periodic payments
      time = i * timeStep;
    }
    cashFlows.push_back({time, couponAmount});
  }

  // Add principal repayment at maturity (combine with last coupon if exists)
  if (!cashFlows.empty()) {
    // Add principal to the last coupon payment
    cashFlows.back().amount += face;
  } else {
    // Zero-coupon case - only principal at maturity
    cashFlows.push_back({maturityYears, face});
  }

  return cashFlows;
}

} // namespace quant