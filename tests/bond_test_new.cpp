#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../core/CashFlow.hpp"
#include "../core/DiscountCurve.hpp"
#include "../engines/Sensitivity.hpp"
#include "../engines/YieldSolver.hpp"
#include "../instruments/Bond.hpp"

using namespace quant;
using Catch::Approx;

TEST_CASE("CashFlow schedule generation", "[cashflow]") {
  SECTION("Semi-annual bond schedule") {
    auto cashFlows =
        bulletSchedule(100.0, 0.06, 2, 2.0); // 6% semi-annual, 2 years

    REQUIRE(cashFlows.size() == 4); // 4 semi-annual payments

    // Check coupon amounts (should be 3.0 per semi-annual period)
    REQUIRE(cashFlows[0].amount == Approx(3.0).margin(0.01));
    REQUIRE(cashFlows[1].amount == Approx(3.0).margin(0.01));
    REQUIRE(cashFlows[2].amount == Approx(3.0).margin(0.01));

    // Last payment should include principal (3.0 + 100.0 = 103.0)
    REQUIRE(cashFlows[3].amount == Approx(103.0).margin(0.01));

    // Check timing (0.5, 1.0, 1.5, 2.0 years)
    REQUIRE(cashFlows[0].time == Approx(0.5).margin(0.01));
    REQUIRE(cashFlows[1].time == Approx(1.0).margin(0.01));
    REQUIRE(cashFlows[2].time == Approx(1.5).margin(0.01));
    REQUIRE(cashFlows[3].time == Approx(2.0).margin(0.01));
  }

  SECTION("Zero-coupon bond") {
    auto cashFlows = bulletSchedule(100.0, 0.0, 1, 1.0); // Zero coupon, 1 year

    REQUIRE(cashFlows.size() == 1);
    REQUIRE(cashFlows[0].amount == Approx(100.0).margin(0.01));
    REQUIRE(cashFlows[0].time == Approx(1.0).margin(0.01));
  }
}

TEST_CASE("Bond pricing", "[bond]") {
  SECTION("Bond price with flat curve") {
    Bond bond(100.0, 0.05, 2, 2.0); // 5% semi-annual, 2 years, face value 100
    DiscountCurve curve(0.04, Compounding::Semi,
                        DayCount::ACT_365F); // 4% semi-annual

    double price = bond.price(curve);

    // Bond should trade at premium (5% coupon vs 4% discount rate)
    REQUIRE(price > 100.0);
    REQUIRE(price == Approx(101.9).margin(1.0));
  }

  SECTION("Zero-coupon bond pricing") {
    Bond zeroBond(100.0, 0.0, 1, 1.0); // Zero coupon, 1 year
    DiscountCurve curve(0.05, Compounding::Annual,
                        DayCount::ACT_365F); // 5% annual

    double price = zeroBond.price(curve);

    // Should be approximately 100/(1.05) = 95.24
    REQUIRE(price == Approx(95.24).margin(0.5));
  }
}

TEST_CASE("Yield from price calculation", "[bond][yield]") {
  SECTION("Par bond should yield coupon rate") {
    Bond bond(100.0, 0.06, 2, 2.0); // 6% semi-annual
    YieldSolver solver;

    double yield = bond.yieldFromPrice(100.0, Compounding::Semi, solver);

    // At par, yield should approximately equal coupon rate
    REQUIRE(yield ==
            Approx(0.06).margin(0.001)); // More precise with new solver
  }

  SECTION("Premium bond should yield less than coupon") {
    Bond bond(100.0, 0.06, 2, 2.0); // 6% semi-annual
    YieldSolver solver;

    double yield = bond.yieldFromPrice(105.0, Compounding::Semi, solver);

    // Premium bond should yield less than coupon rate
    REQUIRE(yield < 0.06);
    REQUIRE(yield == Approx(0.032).margin(0.01)); // More accurate expectation
  }
}

TEST_CASE("Analytic sensitivity calculations", "[sensitivity]") {
  SECTION("Modified duration calculation") {
    std::vector<CashFlow> cashFlows = {
        {0.5, 2.5},  // 6 months coupon
        {1.0, 2.5},  // 1 year coupon
        {1.5, 2.5},  // 18 months coupon
        {2.0, 102.5} // Final coupon + principal
    };

    double yield = 0.05; // 5% yield
    double modDur =
        Sensitivity::modifiedDuration(cashFlows, yield, Compounding::Semi);

    // 2-year bond should have modified duration around 1.9 years
    REQUIRE(modDur > 1.5);
    REQUIRE(modDur < 2.5);
    REQUIRE(modDur == Approx(1.9).margin(0.3));
  }

  SECTION("DV01 calculation") {
    std::vector<CashFlow> cashFlows = {{2.0, 100.0}}; // Simple 2-year zero

    double yield = 0.05;
    double dv01 = Sensitivity::dv01(cashFlows, yield, Compounding::Annual);

    // DV01 should be positive (price sensitivity to 1bp change)
    REQUIRE(dv01 > 0.0);
    REQUIRE(dv01 ==
            Approx(0.017).margin(0.01)); // Rough estimate for 2-year zero
  }

  SECTION("Convexity calculation") {
    std::vector<CashFlow> cashFlows = {
        {1.0, 5.0},  // 1 year coupon
        {2.0, 105.0} // Final coupon + principal
    };

    double yield = 0.05;
    double convexity =
        Sensitivity::convexity(cashFlows, yield, Compounding::Annual);

    // Convexity should be positive for all bonds
    REQUIRE(convexity > 0.0);
  }
}

TEST_CASE("Different compounding conventions", "[compounding]") {
  SECTION("Continuous vs discrete compounding") {
    std::vector<CashFlow> cashFlows = {{1.0, 100.0}}; // 1-year zero
    double yield = 0.05;

    double price_continuous =
        Sensitivity::price(cashFlows, yield, Compounding::Continuous);
    double price_annual =
        Sensitivity::price(cashFlows, yield, Compounding::Annual);

    // Continuous should give slightly lower price (higher effective rate)
    REQUIRE(price_continuous < price_annual);

    // Check approximate values
    REQUIRE(price_continuous == Approx(95.12).margin(0.5)); // e^(-0.05) * 100
    REQUIRE(price_annual == Approx(95.24).margin(0.5));     // 100/1.05
  }

  SECTION("Semi-annual vs annual compounding") {
    std::vector<CashFlow> cashFlows = {{1.0, 100.0}}; // 1-year zero
    double yield = 0.06;

    double price_semi = Sensitivity::price(cashFlows, yield, Compounding::Semi);
    double price_annual =
        Sensitivity::price(cashFlows, yield, Compounding::Annual);

    // Semi-annual should give lower price (higher effective rate)
    REQUIRE(price_semi < price_annual);
  }
}

TEST_CASE("Bond analytics integration", "[bond][analytics]") {
  SECTION("Complete bond analytics") {
    Bond bond(100.0, 0.06, 2, 3.0); // 6% semi-annual, 3 years
    DiscountCurve curve(0.05, Compounding::Semi,
                        DayCount::ACT_365F); // 5% semi-annual

    double price = bond.price(curve);
    double modDur = bond.modDuration(curve, Compounding::Semi);
    double dv01 = bond.dv01(curve, Compounding::Semi);
    double convexity = bond.convexity(curve, Compounding::Semi);

    // Bond should trade at premium
    REQUIRE(price > 100.0);

    // Duration should be reasonable for 3-year bond
    REQUIRE(modDur > 2.0);
    REQUIRE(modDur < 3.0);

    // DV01 should be positive
    REQUIRE(dv01 > 0.0);

    // Convexity should be positive
    REQUIRE(convexity > 0.0);

    // Relationship check: DV01 ≈ price * modDur * 0.0001
    double expected_dv01 = price * modDur * 0.0001;
    REQUIRE(dv01 == Approx(expected_dv01).margin(0.001));
  }
}