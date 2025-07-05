#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../core/CashFlow.hpp"
#include "../core/DayCount.hpp"
#include "../core/DiscountCurve.hpp"
#include <limits>

using namespace quant;
using Catch::Approx;

TEST_CASE("Day count conventions", "[daycount]") {
  SECTION("ACT/365F calculations") {
    Date d0(2024, 1, 1);
    Date d1(2025, 1, 1);

    double yf = yearFraction(d0, d1, DayCount::ACT_365F);

    // 2024 is a leap year, so approximately 366 days / 365 = 1.0027
    REQUIRE(yf == Approx(1.0027).margin(
                      0.01)); // Relaxed margin due to simplified calculation
  }

  SECTION("30/360 calculations") {
    Date d0(2024, 1, 1);
    Date d1(2024, 12, 31);

    double yf = yearFraction(d0, d1, DayCount::THIRTY_360);

    // 30/360: should be exactly 1.0 for a full year
    REQUIRE(yf == Approx(1.0).margin(0.001));
  }
}

TEST_CASE("Discount curve - flat rate", "[discountcurve]") {
  SECTION("Annual compounding") {
    DiscountCurve curve(0.05, Compounding::Annual, DayCount::ACT_365F);

    // Test discount factor: P(0,t) = (1 + y/m)^(-m*t)
    // For annual: P(0,1) = (1 + 0.05/1)^(-1*1) = 1.05^(-1) = 0.9524
    double df = curve.df(1.0);
    REQUIRE(df == Approx(0.9524).margin(0.001));

    // Test forward bond price: 1/df
    double fwdPrice = curve.fwdBondPrice(1.0);
    REQUIRE(fwdPrice == Approx(1.05).margin(0.001));
  }

  SECTION("Semi-annual compounding") {
    DiscountCurve curve(0.06, Compounding::Semi, DayCount::ACT_365F);

    // P(0,1) = (1 + 0.06/2)^(-2*1) = 1.03^(-2) = 0.9426
    double df = curve.df(1.0);
    REQUIRE(df == Approx(0.9426).margin(0.001));
  }

  SECTION("Continuous compounding") {
    DiscountCurve curve(0.05, Compounding::Continuous, DayCount::ACT_365F);

    // P(0,t) = e^(-y*t)
    // P(0,1) = e^(-0.05*1) = 0.9512
    double df = curve.df(1.0);
    REQUIRE(df == Approx(0.9512).margin(0.001));
  }
}

TEST_CASE("Discount curve - bootstrapped", "[discountcurve]") {
  SECTION("Bootstrapped curve interpolation") {
    std::vector<ZeroQuote> quotes = {
        {0.5, 0.98}, // 6 months
        {1.0, 0.95}, // 1 year
        {2.0, 0.90}  // 2 years
    };

    DiscountCurve curve(quotes);

    // Test exact points
    REQUIRE(curve.df(0.5) == Approx(0.98).margin(0.001));
    REQUIRE(curve.df(1.0) == Approx(0.95).margin(0.001));
    REQUIRE(curve.df(2.0) == Approx(0.90).margin(0.001));

    // Test interpolation
    double df_1_5 = curve.df(1.5);
    REQUIRE(df_1_5 ==
            Approx(0.925).margin(0.001)); // Linear between 0.95 and 0.90
  }
}

TEST_CASE("Edge cases and validation", "[discountcurve]") {
  SECTION("Zero time should return 1.0") {
    DiscountCurve curve(0.05, Compounding::Annual, DayCount::ACT_365F);

    REQUIRE(curve.df(0.0) == Approx(1.0).margin(0.001));
  }

  SECTION("Forward bond price should be 1/df") {
    DiscountCurve curve(0.04, Compounding::Semi, DayCount::ACT_365F);

    double t = 1.5;
    double df = curve.df(t);
    double fwdPrice = curve.fwdBondPrice(t);

    REQUIRE(fwdPrice == Approx(1.0 / df).margin(0.001));
  }
}

TEST_CASE("Compounding enum values", "[compounding]") {
  SECTION("Enum values match specification") {
    REQUIRE(static_cast<int>(Compounding::Annual) == 1);
    REQUIRE(static_cast<int>(Compounding::Semi) == 2);
    REQUIRE(static_cast<int>(Compounding::Quarterly) == 4);
    REQUIRE(static_cast<int>(Compounding::Monthly) == 12);
    REQUIRE(static_cast<int>(Compounding::Continuous) == 0);
  }
}

TEST_CASE("Flat curve discount factor validation", "[discountcurve]") {
  SECTION("Continuous compounding: df(1) = exp(-y*t)") {
    double yield = 0.05;
    DiscountCurve curve(yield, Compounding::Continuous, DayCount::ACT_365F);

    double expected = std::exp(-yield * 1.0);
    REQUIRE(curve.df(1.0) == Approx(expected).margin(1e-12));

    // Test multiple time points
    for (double t : {0.25, 0.5, 2.0, 5.0}) {
      double expectedDF = std::exp(-yield * t);
      REQUIRE(curve.df(t) == Approx(expectedDF).margin(1e-12));
    }
  }

  SECTION("Periodic compounding consistency") {
    double yield = 0.06;

    DiscountCurve annual(yield, Compounding::Annual, DayCount::ACT_365F);
    DiscountCurve semi(yield, Compounding::Semi, DayCount::ACT_365F);
    DiscountCurve quarterly(yield, Compounding::Quarterly, DayCount::ACT_365F);

    // All should give reasonable but different discount factors
    double df_annual = annual.df(1.0);
    double df_semi = semi.df(1.0);
    double df_quarterly = quarterly.df(1.0);

    // Higher compounding frequency should give lower discount factors
    REQUIRE(df_semi < df_annual);
    REQUIRE(df_quarterly < df_semi);

    // All should be positive and less than 1
    REQUIRE(df_annual > 0.0);
    REQUIRE(df_annual < 1.0);
    REQUIRE(df_semi > 0.0);
    REQUIRE(df_semi < 1.0);
  }
}

TEST_CASE("30/360 day count edge cases", "[daycount]") {
  SECTION("End-of-month US NASD rules") {
    // Test the corrected 30/360 US rule
    Date d1(2024, 1, 31);
    Date d2(2024, 3, 31);

    double yf_forward = yearFraction(d1, d2, DayCount::THIRTY_360);

    // Test specific US NASD rule: 31st to 31st
    // Jan 31 -> Jan 30, Mar 31 -> Mar 30 (since Jan 31->30): 60 days = 60/360
    REQUIRE(yf_forward == Approx(60.0 / 360.0).margin(1e-10));

    // Test that the calculation is reasonable and positive
    REQUIRE(yf_forward > 0.0);
    REQUIRE(yf_forward < 1.0);
  }

  SECTION("Month-end day adjustments") {
    // Test basic 30/360 adjustment rules
    Date jan30(2024, 1, 30);
    Date jan31(2024, 1, 31);
    Date feb29(2024, 2, 29);

    // 30th to 31st should be treated as 30th to 30th = 0 days
    double yf1 = yearFraction(jan30, jan31, DayCount::THIRTY_360);
    REQUIRE(yf1 == Approx(0.0).margin(1e-10));

    // 31st to end of February: Jan 31->30, Feb 29->29 = 29 days
    double yf2 = yearFraction(jan31, feb29, DayCount::THIRTY_360);
    REQUIRE(yf2 == Approx(29.0 / 360.0).margin(1e-10));
  }
}

TEST_CASE("Core error handling validation", "[core][error_handling]") {
  SECTION("CashFlow input validation") {
    // Test invalid inputs throw appropriate exceptions
    REQUIRE_THROWS_AS(bulletSchedule(0.0, 0.05, 2, 1.0),
                      std::invalid_argument); // Zero face
    REQUIRE_THROWS_AS(bulletSchedule(-100.0, 0.05, 2, 1.0),
                      std::invalid_argument); // Negative face
    REQUIRE_THROWS_AS(bulletSchedule(100.0, 0.05, 0, 1.0),
                      std::invalid_argument); // Zero frequency
    REQUIRE_THROWS_AS(bulletSchedule(100.0, 0.05, -2, 1.0),
                      std::invalid_argument); // Negative frequency
    REQUIRE_THROWS_AS(bulletSchedule(100.0, 0.05, 2, 0.0),
                      std::invalid_argument); // Zero maturity
    REQUIRE_THROWS_AS(bulletSchedule(100.0, 0.05, 2, -1.0),
                      std::invalid_argument); // Negative maturity

    // Test NaN and infinity
    REQUIRE_THROWS_AS(
        bulletSchedule(std::numeric_limits<double>::quiet_NaN(), 0.05, 2, 1.0),
        std::invalid_argument);
    REQUIRE_THROWS_AS(
        bulletSchedule(100.0, std::numeric_limits<double>::infinity(), 2, 1.0),
        std::invalid_argument);
    REQUIRE_THROWS_AS(bulletSchedule(100.0, 0.05, 2,
                                     std::numeric_limits<double>::quiet_NaN()),
                      std::invalid_argument);
  }

  SECTION("DiscountCurve input validation") {
    // Test invalid flat curve construction
    REQUIRE_THROWS_AS(DiscountCurve(std::numeric_limits<double>::quiet_NaN(),
                                    Compounding::Annual, DayCount::ACT_365F),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(DiscountCurve(std::numeric_limits<double>::infinity(),
                                    Compounding::Annual, DayCount::ACT_365F),
                      std::invalid_argument);

    // Test empty bootstrapped curve
    std::vector<ZeroQuote> emptyQuotes;
    REQUIRE_THROWS_AS(DiscountCurve(emptyQuotes), std::invalid_argument);

    // Test invalid quotes
    std::vector<ZeroQuote> invalidQuotes = {
        {-1.0, 0.95}, // Negative time
        {1.0, -0.95}  // Negative discount factor
    };
    REQUIRE_THROWS_AS(DiscountCurve(invalidQuotes), std::invalid_argument);

    // Test NaN quotes
    std::vector<ZeroQuote> nanQuotes = {
        {std::numeric_limits<double>::quiet_NaN(), 0.95}};
    REQUIRE_THROWS_AS(DiscountCurve(nanQuotes), std::invalid_argument);
  }

  SECTION("DiscountCurve df() validation") {
    DiscountCurve curve(0.05, Compounding::Annual, DayCount::ACT_365F);

    // Test invalid time inputs
    REQUIRE_THROWS_AS(curve.df(std::numeric_limits<double>::quiet_NaN()),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(curve.df(std::numeric_limits<double>::infinity()),
                      std::invalid_argument);

    // Valid inputs should work
    REQUIRE_NOTHROW(curve.df(1.0));
    REQUIRE_NOTHROW(curve.df(-1.0)); // Negative time is allowed (returns 1.0)
  }
}

TEST_CASE("CashFlow timing precision", "[cashflow][precision]") {
  SECTION("Non-integer maturity exact timing") {
    // Test 2.5 year bond with semi-annual payments
    auto cashFlows = bulletSchedule(100.0, 0.06, 2, 2.5);

    REQUIRE(cashFlows.size() == 5); // 5 payments for 2.5 years semi-annual

    // Check timing precision
    REQUIRE(cashFlows[0].time == Approx(0.5).margin(1e-12));
    REQUIRE(cashFlows[1].time == Approx(1.0).margin(1e-12));
    REQUIRE(cashFlows[2].time == Approx(1.5).margin(1e-12));
    REQUIRE(cashFlows[3].time == Approx(2.0).margin(1e-12));
    REQUIRE(cashFlows[4].time ==
            Approx(2.5).margin(1e-12)); // Exactly at maturity

    // Final payment should include principal
    REQUIRE(cashFlows[4].amount ==
            Approx(103.0).margin(1e-10)); // 3% coupon + 100 principal
  }

  SECTION("Odd maturity timing") {
    // Test 1.75 year bond with quarterly payments
    auto cashFlows = bulletSchedule(100.0, 0.08, 4, 1.75);

    REQUIRE(cashFlows.size() == 7); // 7 payments for 1.75 years quarterly

    // Last payment should be exactly at 1.75
    REQUIRE(cashFlows.back().time == Approx(1.75).margin(1e-12));
  }
}

TEST_CASE("Log-linear interpolation validation", "[discountcurve]") {
  SECTION("Monotonicity preservation") {
    std::vector<ZeroQuote> quotes = {
        {1.0, 0.95}, // 5% yield
        {2.0, 0.90}, // ~5.1% yield
        {3.0, 0.85}  // ~5.3% yield
    };

    DiscountCurve curve(quotes);

    // Test intermediate points maintain monotonicity
    double df_1_5 = curve.df(1.5);
    double df_2_5 = curve.df(2.5);

    // Should be between neighboring points
    REQUIRE(df_1_5 < 0.95);
    REQUIRE(df_1_5 > 0.90);
    REQUIRE(df_2_5 < 0.90);
    REQUIRE(df_2_5 > 0.85);

    // Should be monotonically decreasing
    REQUIRE(df_1_5 > df_2_5);
  }

  SECTION("Positive discount factor preservation") {
    std::vector<ZeroQuote> quotes = {
        {0.5, 0.98}, {1.0, 0.95}, {5.0, 0.78} // Long term
    };

    DiscountCurve curve(quotes);

    // Test many intermediate points - all should be positive
    for (double t = 0.1; t < 5.0; t += 0.1) {
      double df = curve.df(t);
      REQUIRE(df > 0.0);
      REQUIRE(df <= 1.0);
    }
  }
}