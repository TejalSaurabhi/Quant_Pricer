#include "EuropeanBondOption.hpp"
#include "../engines/MonteCarlo.hpp"
#include <cmath>
#include <random>

namespace quant {

EuropeanBondOption::EuropeanBondOption(Type type, double strike, double expiry)
    : type_(type), K_(strike), T_(expiry) {}

double EuropeanBondOption::priceBlack(const DiscountCurve &curve,
                                      double sigma) const {
  // Get forward price of bond that matures 5 years after option expiry
  double forwardPrice = getForwardPrice(curve);

  // Get discount factor to option expiry
  double discountFactor = curve.df(T_);

  // Use Black-76 formula
  bool isCall = (type_ == Type::Call);
  return Black76::price(forwardPrice, K_, T_, sigma, discountFactor, isCall);
}

double EuropeanBondOption::priceMC(const DiscountCurve &curve, double sigma,
                                   std::size_t paths) const {

  // Use the new vectorized Monte Carlo engine
  double forwardPrice = getForwardPrice(curve);
  double discountFactor = curve.df(T_);

  // Convert EuropeanBondOption::Type to MonteCarlo::OptionType
  quant::OptionType mcType =
      (type_ == Type::Call) ? quant::OptionType::Call : quant::OptionType::Put;

  return MonteCarlo::mcPrice(forwardPrice, K_, sigma, T_, discountFactor,
                             mcType, paths);
}

double EuropeanBondOption::vegaBlack(const DiscountCurve &curve,
                                     double sigma) const {
  // Get forward price and discount factor
  double forwardPrice = getForwardPrice(curve);
  double discountFactor = curve.df(T_);

  // Use Black-76 vega calculation
  return Black76::vega(forwardPrice, K_, T_, sigma, discountFactor);
}

double EuropeanBondOption::getForwardPrice(const DiscountCurve &curve) const {
  // Underlying forward price = curve.fwdBondPrice(T_ + maturity-of-bond)
  // For demo, bond maturity is T + 5 years
  double bondMaturity = T_ + 5.0;
  return curve.fwdBondPrice(bondMaturity);
}

} // namespace quant