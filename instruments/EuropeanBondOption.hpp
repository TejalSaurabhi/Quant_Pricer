#pragma once
#include "../core/DiscountCurve.hpp"
#include "../engines/Black76.hpp"
#include <cstddef>

namespace quant {

class EuropeanBondOption {
public:
  enum class Type { Call, Put };

  EuropeanBondOption(Type type, double strike, double expiry);

  double priceBlack(const DiscountCurve &curve, double sigma) const;
  double priceMC(const DiscountCurve &curve, double sigma,
                 std::size_t paths = 100'000) const;
  double vegaBlack(const DiscountCurve &curve, double sigma) const;

private:
  Type type_;
  double K_; // Strike price
  double T_; // Expiry time

  // Helper: Get underlying forward price
  // Forward price = curve.fwdBondPrice(T_ + 5.0) for bond maturing 5 years
  // after option expiry
  double getForwardPrice(const DiscountCurve &curve) const;
};

} // namespace quant