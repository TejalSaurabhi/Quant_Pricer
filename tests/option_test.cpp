#include "../core/DiscountCurve.hpp"
#include "../engines/Black76.hpp"
#include "../engines/MonteCarlo.hpp"
#include "../instruments/EuropeanBondOption.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace quant;
using Catch::Approx;

TEST_CASE("Monte Carlo vs Black-76 Statistical Validation",
          "[monte_carlo][black76][statistical]") {

  SECTION("Call option: MC within 1.5 × std-error") {
    double F0 = 1.3, K = 1.25, sigma = 0.20, T = 1.0, df = 0.95;
    std::size_t N = 1000000; // Large sample for good statistics

    // Black-76 analytical price
    double blackPrice = Black76::price(F0, K, T, sigma, df, true);

    // Monte Carlo with statistics
    MonteCarlo::MCResult result =
        MonteCarlo::mcPriceWithStats(F0, K, sigma, T, df, OptionType::Call, N);

    double mcPrice = result.price;
    double stdError = result.standardError;
    double difference = std::abs(mcPrice - blackPrice);

    INFO("Black-76 price: " << blackPrice);
    INFO("Monte Carlo price: " << mcPrice);
    INFO("Standard error: " << stdError);
    INFO("Difference: " << difference);
    INFO("1.5 × std-error: " << 1.5 * stdError);

    // Key requirement: MC mean within 1.5 × standard error
    REQUIRE(difference < 1.5 * stdError);

    // Additional quality checks
    REQUIRE(stdError > 0.0);
    REQUIRE(stdError < 0.01); // Should have reasonable precision
    REQUIRE(result.effectivePaths > 0);
  }

  SECTION("Put option: MC within 1.5 × std-error") {
    double F0 = 1.2, K = 1.25, sigma = 0.25, T = 0.5, df = 0.97;
    std::size_t N = 500000;

    // Black-76 analytical price
    double blackPrice = Black76::price(F0, K, T, sigma, df, false);

    // Monte Carlo with statistics
    MonteCarlo::MCResult result =
        MonteCarlo::mcPriceWithStats(F0, K, sigma, T, df, OptionType::Put, N);

    double mcPrice = result.price;
    double stdError = result.standardError;
    double difference = std::abs(mcPrice - blackPrice);

    INFO("Black-76 price: " << blackPrice);
    INFO("Monte Carlo price: " << mcPrice);
    INFO("Standard error: " << stdError);
    INFO("Difference: " << difference);
    INFO("1.5 × std-error: " << 1.5 * stdError);

    // Key requirement: MC mean within 1.5 × standard error
    REQUIRE(difference < 1.5 * stdError);
  }

  SECTION("Different volatilities: Statistical consistency") {
    double F0 = 1.35, K = 1.30, T = 2.0, df = 0.90;
    std::size_t N = 800000;

    std::vector<double> vols = {0.10, 0.15, 0.20, 0.30, 0.40};

    for (double sigma : vols) {
      double blackPrice = Black76::price(F0, K, T, sigma, df, true);

      MonteCarlo::MCResult result = MonteCarlo::mcPriceWithStats(
          F0, K, sigma, T, df, OptionType::Call, N);

      double difference = std::abs(result.price - blackPrice);

      INFO("Volatility: " << sigma);
      INFO("Black-76: " << blackPrice);
      INFO("Monte Carlo: " << result.price);
      INFO("Std Error: " << result.standardError);

      REQUIRE(difference < 1.5 * result.standardError);
    }
  }

  SECTION("At-the-money vs out-of-the-money: Statistical validation") {
    double F0 = 1.30, sigma = 0.20, T = 1.0, df = 0.95;
    std::vector<double> strikes = {1.20, 1.25, 1.30, 1.35, 1.40};
    std::size_t N = 750000;

    for (double K : strikes) {
      // Test both calls and puts
      for (bool isCall : {true, false}) {
        auto optType = isCall ? OptionType::Call : OptionType::Put;

        double blackPrice = Black76::price(F0, K, T, sigma, df, isCall);

        MonteCarlo::MCResult result =
            MonteCarlo::mcPriceWithStats(F0, K, sigma, T, df, optType, N);

        double difference = std::abs(result.price - blackPrice);

        INFO("Strike: " << K << " (" << (isCall ? "Call" : "Put") << ")");
        INFO("Moneyness: " << F0 / K);
        INFO("Black-76: " << blackPrice);
        INFO("Monte Carlo: " << result.price);
        INFO("Std Error: " << result.standardError);

        REQUIRE(difference < 1.5 * result.standardError);
      }
    }
  }
}

TEST_CASE("Black76 Formula Components", "[black76]") {

  SECTION("Basic Black-76 pricing") {
    double F = 100.0;
    double K = 100.0;
    double T = 1.0;
    double sigma = 0.20;
    double D = 0.95;

    double callPrice = Black76::price(F, K, T, sigma, D, true);
    double putPrice = Black76::price(F, K, T, sigma, D, false);

    REQUIRE(callPrice > 0.0);
    REQUIRE(putPrice > 0.0);

    // Put-call parity: Call - Put = D * (F - K)
    double parity = callPrice - putPrice;
    double expected = D * (F - K);
    REQUIRE(std::abs(parity - expected) < 1e-10);
  }

  SECTION("Greeks calculations") {
    double F = 100.0, K = 100.0, T = 1.0, sigma = 0.20, D = 0.95;

    double callDelta = Black76::delta(F, K, T, sigma, D, true);
    double putDelta = Black76::delta(F, K, T, sigma, D, false);
    double vega = Black76::vega(F, K, T, sigma, D);

    REQUIRE(callDelta > 0.0);
    REQUIRE(callDelta < D);
    REQUIRE(putDelta < 0.0);
    REQUIRE(vega > 0.0);

    // Delta relationship: Call delta - Put delta = D
    REQUIRE(std::abs((callDelta - putDelta) - D) < 1e-10);
  }
}

TEST_CASE("EuropeanBondOption Interface", "[bond_option]") {

  DiscountCurve curve(0.05, Compounding::Annual, DayCount::ACT_365F);

  SECTION("Black-76 vs Monte Carlo convergence") {
    EuropeanBondOption call(EuropeanBondOption::Type::Call, 1.30, 1.0);
    double sigma = 0.25;

    double blackPrice = call.priceBlack(curve, sigma);
    double mcPrice1 = call.priceMC(curve, sigma, 100000);
    double mcPrice2 = call.priceMC(curve, sigma, 1000000);

    // Monte Carlo should converge to Black-76 price
    REQUIRE(std::abs(mcPrice1 - blackPrice) < 0.01);
    REQUIRE(std::abs(mcPrice2 - blackPrice) < 0.005);
  }

  SECTION("Put-call parity") {
    double strike = 1.25;
    double expiry = 0.5;

    EuropeanBondOption call(EuropeanBondOption::Type::Call, strike, expiry);
    EuropeanBondOption put(EuropeanBondOption::Type::Put, strike, expiry);

    double callPrice = call.priceBlack(curve, 0.20);
    double putPrice = put.priceBlack(curve, 0.20);

    // Put-call parity: Call - Put = D * (F - K)
    double forwardPrice = curve.fwdBondPrice(expiry + 5.0);
    double discountFactor = curve.df(expiry);
    double expectedDiff = discountFactor * (forwardPrice - strike);

    REQUIRE(std::abs((callPrice - putPrice) - expectedDiff) < 1e-10);
  }
}

TEST_CASE("Monte Carlo Engine Validation", "[monte_carlo]") {

  SECTION("Antithetic variates effectiveness") {
    double F0 = 1.3, K = 1.25, sigma = 0.20, T = 1.0, df = 0.95;
    std::size_t N = 100000;

    MonteCarlo::Config configStandard;
    configStandard.useAntithetic = false;
    configStandard.randomSeed = 42;

    MonteCarlo::Config configAntithetic;
    configAntithetic.useAntithetic = true;
    configAntithetic.randomSeed = 42;

    double mcStandard = MonteCarlo::mcPriceAdvanced(
        F0, K, sigma, T, df, OptionType::Call, N, configStandard);

    double mcAntithetic = MonteCarlo::mcPriceAdvanced(
        F0, K, sigma, T, df, OptionType::Call, N, configAntithetic);

    double blackPrice = Black76::price(F0, K, T, sigma, df, true);

    REQUIRE(std::abs(mcStandard - blackPrice) < 0.01);
    REQUIRE(std::abs(mcAntithetic - blackPrice) < 0.01);
  }

  SECTION("Vectorization consistency") {
    double F0 = 1.2, K = 1.25, sigma = 0.25, T = 1.5, df = 0.93;
    std::size_t N = 50000;

    MonteCarlo::Config configVector;
    configVector.enableVectorization = true;
    configVector.batchSize = 8000;
    configVector.randomSeed = 123;

    MonteCarlo::Config configScalar;
    configScalar.enableVectorization = false;
    configScalar.batchSize = 1;
    configScalar.randomSeed = 123;

    double mcVector = MonteCarlo::mcPriceAdvanced(
        F0, K, sigma, T, df, OptionType::Put, N, configVector);

    double mcScalar = MonteCarlo::mcPriceAdvanced(
        F0, K, sigma, T, df, OptionType::Put, N, configScalar);

    // With same seed, vectorized and scalar should give identical results
    REQUIRE(std::abs(mcVector - mcScalar) < 1e-12);
  }
}