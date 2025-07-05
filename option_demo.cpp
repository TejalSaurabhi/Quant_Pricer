#include "core/DiscountCurve.hpp"
#include "engines/Black76.hpp"
#include "instruments/EuropeanBondOption.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace quant;

int main() {
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "=== European Bond Option Demo ===\n";
  std::cout << "Black-76 Model: V = D[F*N(d1) - K*N(d2)]\n";
  std::cout << "d1,2 = [ln(F/K) ± 0.5σ²T] / (σ√T)\n\n";

  // Setup: 5% flat yield curve
  DiscountCurve curve(0.05, Compounding::Annual, DayCount::ACT_365F);

  // Option parameters
  double expiry = 1.0;      // 1 year to option expiry
  double strike = 1.25;     // Strike price (closer to forward price)
  double volatility = 0.20; // 20% volatility

  std::cout << "Market Setup:\n";
  std::cout << "  Yield curve: 5% flat (annual compounding)\n";
  std::cout << "  Option expiry: " << expiry << " year\n";
  std::cout << "  Strike price: ₹" << strike << " (near-the-money)\n";
  std::cout << "  Volatility: " << (volatility * 100) << "%\n";
  std::cout << "  Bond maturity: " << (expiry + 5.0)
            << " years (5Y after option expiry)\n\n";

  // Calculate underlying parameters
  double bondMaturity = expiry + 5.0;
  double forwardPrice = curve.fwdBondPrice(bondMaturity);
  double discountFactor = curve.df(expiry);

  std::cout << "Underlying Bond Forward:\n";
  std::cout << "  Bond maturity time: " << bondMaturity << " years\n";
  std::cout << "  Forward price: ₹" << forwardPrice << " = 1/df("
            << bondMaturity << ")\n";
  std::cout << "  Discount factor to expiry: " << discountFactor << " = df("
            << expiry << ")\n\n";

  // Black-76 formula components
  double F = forwardPrice;
  double K = strike;
  double T = expiry;
  double sigma = volatility;
  double D = discountFactor;

  double d1 =
      (std::log(F / K) + 0.5 * sigma * sigma * T) / (sigma * std::sqrt(T));
  double d2 = d1 - sigma * std::sqrt(T);
  double N_d1 = 0.5 * (1.0 + std::erf(d1 / std::sqrt(2.0)));
  double N_d2 = 0.5 * (1.0 + std::erf(d2 / std::sqrt(2.0)));

  std::cout << "Black-76 Parameters:\n";
  std::cout << "  F (forward): ₹" << F << "\n";
  std::cout << "  K (strike): ₹" << K << "\n";
  std::cout << "  T (time): " << T << " years\n";
  std::cout << "  σ (vol): " << sigma << "\n";
  std::cout << "  D (discount): " << D << "\n";
  std::cout << "  d1: " << d1 << "\n";
  std::cout << "  d2: " << d2 << "\n";
  std::cout << "  N(d1): " << N_d1 << "\n";
  std::cout << "  N(d2): " << N_d2 << "\n\n";

  // Create options
  EuropeanBondOption call(EuropeanBondOption::Type::Call, strike, expiry);
  EuropeanBondOption put(EuropeanBondOption::Type::Put, strike, expiry);

  // Price options using Black-76
  double callPriceBlack = call.priceBlack(curve, volatility);
  double putPriceBlack = put.priceBlack(curve, volatility);

  // Manual calculation for verification
  double callPriceManual = D * (F * N_d1 - K * N_d2);
  double putPriceManual = D * (K * (1.0 - N_d2) - F * (1.0 - N_d1));

  std::cout << "Call Option Pricing:\n";
  std::cout << "  Black-76 class: ₹" << callPriceBlack << "\n";
  std::cout << "  Manual calc: ₹" << callPriceManual << " = " << D << " × ("
            << F << " × " << N_d1 << " - " << K << " × " << N_d2 << ")\n";
  std::cout << "  Difference: ₹" << std::abs(callPriceBlack - callPriceManual)
            << "\n\n";

  std::cout << "Put Option Pricing:\n";
  std::cout << "  Black-76 class: ₹" << putPriceBlack << "\n";
  std::cout << "  Manual calc: ₹" << putPriceManual << "\n";
  std::cout << "  Difference: ₹" << std::abs(putPriceBlack - putPriceManual)
            << "\n\n";

  // Monte Carlo pricing
  std::cout << "Monte Carlo Verification:\n";
  std::size_t paths = 1000000;
  double callPriceMC = call.priceMC(curve, volatility, paths);
  double putPriceMC = put.priceMC(curve, volatility, paths);

  std::cout << "  Call (MC " << paths << " paths): ₹" << callPriceMC << "\n";
  std::cout << "  Call (Black-76): ₹" << callPriceBlack << "\n";
  std::cout << "  MC vs Black error: ₹"
            << std::abs(callPriceMC - callPriceBlack) << "\n\n";

  std::cout << "  Put (MC " << paths << " paths): ₹" << putPriceMC << "\n";
  std::cout << "  Put (Black-76): ₹" << putPriceBlack << "\n";
  std::cout << "  MC vs Black error: ₹" << std::abs(putPriceMC - putPriceBlack)
            << "\n\n";

  // Greeks calculation
  double callVega = call.vegaBlack(curve, volatility);
  double putVega = put.vegaBlack(curve, volatility);

  // Manual vega calculation
  double phi_d1 = (1.0 / std::sqrt(2.0 * M_PI)) * std::exp(-0.5 * d1 * d1);
  double vegaManual = D * F * phi_d1 * std::sqrt(T);

  std::cout << "Greeks:\n";
  std::cout << "  Call Vega: " << callVega << "\n";
  std::cout << "  Put Vega: " << putVega << "\n";
  std::cout << "  Manual Vega: " << vegaManual << " = " << D << " × " << F
            << " × " << phi_d1 << " × √" << T << "\n";
  std::cout << "  Vega difference: " << std::abs(callVega - vegaManual)
            << "\n\n";

  // Put-call parity verification
  // Call - Put = D * (F - K)
  double putCallParity = callPriceBlack - putPriceBlack;
  double expectedParity = D * (F - K);

  std::cout << "Put-Call Parity:\n";
  std::cout << "  Call - Put: ₹" << putCallParity << "\n";
  std::cout << "  D*(F-K): ₹" << expectedParity << " = " << D << " × (" << F
            << " - " << K << ")\n";
  std::cout << "  Parity error: ₹" << std::abs(putCallParity - expectedParity)
            << "\n\n";

  // Sensitivity to different parameters
  std::cout << "Parameter Sensitivity:\n";
  std::vector<double> vols = {0.10, 0.15, 0.20, 0.25, 0.30};
  std::cout << "Volatility\tCall Price\tPut Price\tCall Vega\n";
  for (double vol : vols) {
    double cPrice = call.priceBlack(curve, vol);
    double pPrice = put.priceBlack(curve, vol);
    double cVega = call.vegaBlack(curve, vol);
    std::cout << std::setw(8) << (vol * 100) << "%\t₹" << std::setw(8) << cPrice
              << "\t₹" << std::setw(8) << pPrice << "\t" << std::setw(8)
              << cVega << "\n";
  }

  std::cout << "\n=== Demo Complete ===\n";
  std::cout << "Implementation Summary:\n";
  std::cout << "✓ Black-76 formula: V = D[F*N(d1) - K*N(d2)]\n";
  std::cout << "✓ d1,2 = [ln(F/K) ± 0.5σ²T] / (σ√T)\n";
  std::cout << "✓ Monte Carlo convergence\n";
  std::cout << "✓ Greeks calculation\n";
  std::cout << "✓ Put-call parity\n";

  return 0;
}