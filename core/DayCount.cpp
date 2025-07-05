#include "DayCount.hpp"
#include <algorithm>
#include <utility>

namespace quant {

// Helper function to convert Date to chrono::year_month_day when available
#if __cpp_lib_chrono >= 201907L
std::chrono::year_month_day dateToYMD(const Date &d) {
  return std::chrono::year_month_day{
      std::chrono::year{d.year},
      std::chrono::month{static_cast<unsigned>(d.month)},
      std::chrono::day{static_cast<unsigned>(d.day)}};
}
#endif

// Core implementation using simple Date struct
double yearFraction(Date d0, Date d1, DayCount dc) {
  // Ensure d0 <= d1 (simple comparison)
  if (d0.year > d1.year || (d0.year == d1.year && d0.month > d1.month) ||
      (d0.year == d1.year && d0.month == d1.month && d0.day > d1.day)) {
    std::swap(d0, d1);
  }

  switch (dc) {
  case DayCount::ACT_365F: {
    // Calculate actual days using simple date arithmetic
    // This is an approximation for cross-platform compatibility
    int totalDays = 0;

    // Year difference
    totalDays += (d1.year - d0.year) * 365;

    // Add leap years
    for (int year = d0.year; year < d1.year; ++year) {
      if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        totalDays += 1;
      }
    }

    // Month and day difference (simplified)
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

    // Subtract start month days
    for (int m = 1; m < d0.month; ++m) {
      totalDays -= daysInMonth[m - 1];
      if (m == 2 &&
          ((d0.year % 4 == 0 && d0.year % 100 != 0) || (d0.year % 400 == 0))) {
        totalDays -= 1; // Leap year February
      }
    }
    totalDays -= (d0.day - 1);

    // Add end month days
    for (int m = 1; m < d1.month; ++m) {
      totalDays += daysInMonth[m - 1];
      if (m == 2 &&
          ((d1.year % 4 == 0 && d1.year % 100 != 0) || (d1.year % 400 == 0))) {
        totalDays += 1; // Leap year February
      }
    }
    totalDays += (d1.day - 1);

    return totalDays / 365.0;
  }

  case DayCount::THIRTY_360: {
    // 30/360 US (NASD) convention
    int y0 = d0.year;
    int m0 = d0.month;
    int d0_day = d0.day;

    int y1 = d1.year;
    int m1 = d1.month;
    int d1_day = d1.day;

    // Adjust days according to 30/360 US (NASD) rules
    // Rule 1: If start date is 31st, change to 30th
    if (d0_day == 31) {
      d0_day = 30;
    }
    // Rule 2: If end date is 31st AND start date is now 30th (after rule 1),
    // change end to 30th
    if (d1_day == 31 && d0_day == 30) {
      d1_day = 30;
    }

    int days = 360 * (y1 - y0) + 30 * (m1 - m0) + (d1_day - d0_day);
    return days / 360.0;
  }

  default:
    // Should never reach here with valid enum values
    return 0.0;
  }
}

#if __cpp_lib_chrono >= 201907L
// C++20 chrono version when available
double yearFraction(std::chrono::year_month_day d0,
                    std::chrono::year_month_day d1, DayCount dc) {
  // Convert to simple Date struct and delegate
  Date d0_simple(static_cast<int>(d0.year()),
                 static_cast<int>(static_cast<unsigned>(d0.month())),
                 static_cast<int>(static_cast<unsigned>(d0.day())));
  Date d1_simple(static_cast<int>(d1.year()),
                 static_cast<int>(static_cast<unsigned>(d1.month())),
                 static_cast<int>(static_cast<unsigned>(d1.day())));

  return yearFraction(d0_simple, d1_simple, dc);
}
#endif

} // namespace quant