#pragma once
#include <chrono>
#include <cmath>

namespace quant {

enum class DayCount { ACT_365F, THIRTY_360 };

// Simple date structure for cross-platform compatibility
struct Date {
  int year;
  int month;
  int day;

  Date(int y, int m, int d) : year(y), month(m), day(d) {}
};

// Overloaded function for compatibility
double yearFraction(Date d0, Date d1, DayCount dc);

// C++20 version when available
#if __cpp_lib_chrono >= 201907L
double yearFraction(std::chrono::year_month_day d0,
                    std::chrono::year_month_day d1, DayCount dc);
#endif

} // namespace quant