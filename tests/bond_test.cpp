#include "../core/DiscountCurve.hpp"
#include "../engines/YieldSolver.hpp"
#include "../instruments/Bond.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace quant;
using Catch::Approx;

TEST_CASE("Bond Construction and Basic Properties", "[bond]") {
  SECTION("Bond construction") {
    Bond bond(100.0, 0.05, 2, 5.0); // 5% semi-annual, 5Y maturity

    // Should construct without throwing
    REQUIRE(true);
  }

  SECTION("Bond pricing with different curves") {
    Bond bond(100.0, 0.06, 2, 3.0); // 6% semi-annual, 3Y maturity

    DiscountCurve flatLow(0.04, Compounding::Semi, DayCount::ACT_365F);
    DiscountCurve flatHigh(0.08, Compounding::Semi, DayCount::ACT_365F);

    double priceLow = bond.price(flatLow);
    double priceHigh = bond.price(flatHigh);

    // Price should be higher when discount rate is lower
    REQUIRE(priceLow > priceHigh);

    // Prices should be positive and reasonable
    REQUIRE(priceLow > 80.0);
    REQUIRE(priceLow < 150.0);
    REQUIRE(priceHigh > 80.0);
    REQUIRE(priceHigh < 150.0);
  }
}

TEST_CASE("Bond Analytics - Duration and Convexity", "[bond]") {
  Bond bond(100.0, 0.05, 2, 5.0); // 5% semi-annual, 5Y
  DiscountCurve curve(0.06, Compounding::Semi, DayCount::ACT_365F);

  SECTION("Modified duration properties") {
    double duration = bond.modDuration(curve, Compounding::Semi);

    REQUIRE(duration > 0.0);
    REQUIRE(duration < 10.0); // Should be reasonable for 5Y bond

    // Duration should be roughly around maturity for bonds trading near par
    REQUIRE(duration > 3.0);
    REQUIRE(duration < 6.0);
  }

  SECTION("Convexity properties") {
    double convexity = bond.convexity(curve, Compounding::Semi);

    REQUIRE(convexity > 0.0);   // Convexity should always be positive
    REQUIRE(convexity < 100.0); // Should be reasonable
  }

  SECTION("DV01 properties") {
    double dv01 = bond.dv01(curve, Compounding::Semi);

    REQUIRE(dv01 > 0.0); // DV01 should be positive
    REQUIRE(dv01 < 1.0); // Should be less than face value
  }
}

TEST_CASE("Bond Greeks: Analytical vs Finite Difference", "[bond][greeks]") {
  DiscountCurve flat(0.06, Compounding::Semi, DayCount::ACT_365F);
  Bond b(100.0, 0.08, 2, 5.0); // 8% semi-annual, 5Y bond

  SECTION("DV01 analytical vs finite difference") {
    for (auto m : {1, 2, 4, 12, 0}) {
      double dvAna = b.dv01(flat, static_cast<Compounding>(m));
      double eps = 1e-6;

      DiscountCurve up(0.06 + eps, static_cast<Compounding>(m),
                       DayCount::ACT_365F);
      DiscountCurve dn(0.06 - eps, static_cast<Compounding>(m),
                       DayCount::ACT_365F);

      double dvFD = (b.price(dn) - b.price(up)) / (2 * eps) / 1e4;

      INFO("Compounding frequency: " << m);
      INFO("Analytical DV01: " << dvAna);
      INFO("Finite Diff DV01: " << dvFD);
      INFO("Difference: " << std::abs(dvAna - dvFD));

      REQUIRE(std::abs(dvAna - dvFD) < 1e-3);
    }
  }

  SECTION("Duration finite difference validation") {
    Bond bond(100.0, 0.07, 2, 4.0);
    DiscountCurve baseCurve(0.05, Compounding::Semi, DayCount::ACT_365F);

    double analyticalDuration = bond.modDuration(baseCurve, Compounding::Semi);

    // Finite difference duration calculation
    double eps = 1e-4;
    DiscountCurve upCurve(0.05 + eps, Compounding::Semi, DayCount::ACT_365F);
    DiscountCurve dnCurve(0.05 - eps, Compounding::Semi, DayCount::ACT_365F);

    double basePrice = bond.price(baseCurve);
    double upPrice = bond.price(upCurve);
    double dnPrice = bond.price(dnCurve);

    double fdDuration = -(upPrice - dnPrice) / (2 * eps) / basePrice;

    INFO("Analytical Duration: " << analyticalDuration);
    INFO("Finite Diff Duration: " << fdDuration);

    REQUIRE(std::abs(analyticalDuration - fdDuration) < 1e-3);
  }

  SECTION("Convexity finite difference validation") {
    Bond bond(100.0, 0.06, 2, 3.0);
    DiscountCurve baseCurve(0.05, Compounding::Semi, DayCount::ACT_365F);

    double analyticalConvexity = bond.convexity(baseCurve, Compounding::Semi);

    // Finite difference convexity calculation
    double eps = 1e-4;
    DiscountCurve upCurve(0.05 + eps, Compounding::Semi, DayCount::ACT_365F);
    DiscountCurve dnCurve(0.05 - eps, Compounding::Semi, DayCount::ACT_365F);

    double basePrice = bond.price(baseCurve);
    double upPrice = bond.price(upCurve);
    double dnPrice = bond.price(dnCurve);

    double fdConvexity =
        (upPrice + dnPrice - 2 * basePrice) / (eps * eps) / basePrice;

    INFO("Analytical Convexity: " << analyticalConvexity);
    INFO("Finite Diff Convexity: " << fdConvexity);

    REQUIRE(std::abs(analyticalConvexity - fdConvexity) < 5.0);
  }
}

TEST_CASE("Bond Yield Calculations", "[bond][yield]") {
  SECTION("Yield-to-maturity round trip") {
    Bond bond(100.0, 0.05, 2, 4.0);
    DiscountCurve curve(0.06, Compounding::Semi, DayCount::ACT_365F);

    double price = bond.price(curve);

    YieldSolver solver;
    double yield = bond.yieldFromPrice(price, Compounding::Semi, solver);

    // The yield should be close to the curve rate (6%)
    REQUIRE(yield == Approx(0.06).margin(1e-6));
  }

  SECTION("Par bond yield equals coupon rate") {
    double couponRate = 0.07;
    Bond bond(100.0, couponRate, 2, 5.0);

    // Create curve that prices bond at par
    DiscountCurve parCurve(couponRate, Compounding::Semi, DayCount::ACT_365F);

    double price = bond.price(parCurve);

    // Price should be approximately par (100)
    REQUIRE(price == Approx(100.0).margin(1e-10));

    YieldSolver solver;
    double yield = bond.yieldFromPrice(price, Compounding::Semi, solver);

    // Yield should equal coupon rate
    REQUIRE(yield == Approx(couponRate).margin(1e-8));
  }
}

TEST_CASE("Bond Edge Cases", "[bond][edge]") {
  SECTION("Zero coupon bond") {
    Bond zeroCoupon(100.0, 0.0, 1, 5.0);
    DiscountCurve curve(0.05, Compounding::Annual, DayCount::ACT_365F);

    double price = zeroCoupon.price(curve);

    // Zero coupon bond price should be discounted face value
    double expectedPrice = 100.0 * std::pow(1.05, -5.0);
    REQUIRE(price == Approx(expectedPrice).margin(1e-10));
  }

  SECTION("Very short maturity bond") {
    Bond shortBond(100.0, 0.05, 4, 0.25); // 3 months
    DiscountCurve curve(0.04, Compounding::Quarterly, DayCount::ACT_365F);

    double price = shortBond.price(curve);

    // Should be close to par plus accrued interest
    REQUIRE(price > 99.0);
    REQUIRE(price < 102.0);
  }

  SECTION("High coupon bond") {
    Bond highCoupon(100.0, 0.15, 2, 10.0); // 15% coupon
    DiscountCurve lowYieldCurve(0.03, Compounding::Semi, DayCount::ACT_365F);

    double price = highCoupon.price(lowYieldCurve);

    // High coupon bond with low yield should trade at premium
    REQUIRE(price > 150.0);
  }

  SECTION("Different compounding frequencies") {
    Bond bond(100.0, 0.06, 2, 3.0);

    DiscountCurve annualCurve(0.05, Compounding::Annual, DayCount::ACT_365F);
    DiscountCurve monthlyCurve(0.05, Compounding::Monthly, DayCount::ACT_365F);
    DiscountCurve continuousCurve(0.05, Compounding::Continuous,
                                  DayCount::ACT_365F);

    double priceAnnual = bond.price(annualCurve);
    double priceMonthly = bond.price(monthlyCurve);
    double priceContinuous = bond.price(continuousCurve);

    // All prices should be positive and within reasonable range
    REQUIRE(priceAnnual > 90.0);
    REQUIRE(priceAnnual < 120.0);
    REQUIRE(priceMonthly > 90.0);
    REQUIRE(priceMonthly < 120.0);
    REQUIRE(priceContinuous > 90.0);
    REQUIRE(priceContinuous < 120.0);

    // Different compounding should give slightly different prices
    // but they should all be reasonable
    INFO("Annual: " << priceAnnual);
    INFO("Monthly: " << priceMonthly);
    INFO("Continuous: " << priceContinuous);
  }
}

TEST_CASE("Bond Price Sensitivity Tests", "[bond][sensitivity]") {
  Bond bond(100.0, 0.05, 2, 5.0);

  SECTION("Price decreases with yield increase") {
    DiscountCurve lowYield(0.03, Compounding::Semi, DayCount::ACT_365F);
    DiscountCurve highYield(0.08, Compounding::Semi, DayCount::ACT_365F);

    double priceLow = bond.price(lowYield);
    double priceHigh = bond.price(highYield);

    REQUIRE(priceLow > priceHigh);

    // Test magnitude is reasonable
    REQUIRE(priceLow - priceHigh > 5.0); // Should have meaningful sensitivity
  }

  SECTION("DV01 increases with duration") {
    Bond shortBond(100.0, 0.05, 2, 2.0);
    Bond longBond(100.0, 0.05, 2, 10.0);

    DiscountCurve curve(0.05, Compounding::Semi, DayCount::ACT_365F);

    double dv01Short = shortBond.dv01(curve, Compounding::Semi);
    double dv01Long = longBond.dv01(curve, Compounding::Semi);

    REQUIRE(dv01Long > dv01Short);
  }

  SECTION("Greeks consistency") {
    DiscountCurve curve(0.05, Compounding::Semi, DayCount::ACT_365F);

    double price = bond.price(curve);
    double duration = bond.modDuration(curve, Compounding::Semi);
    double dv01 = bond.dv01(curve, Compounding::Semi);

    // Relationship: DV01 ≈ Duration * Price / 10000
    double expectedDV01 = duration * price / 10000.0;

    REQUIRE(dv01 == Approx(expectedDV01).margin(1e-6));
  }
}