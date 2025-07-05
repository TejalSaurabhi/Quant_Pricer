#pragma once
#include "DayCount.hpp"
#include <cmath>
#include <vector>

// Use std::span when available, fallback to vector view
#if __cpp_lib_span >= 202002L
#include <span>
#define QUANT_SPAN std::span
#else
// Simple span-like view for compatibility
template <typename T> class span_view {
public:
  span_view(const T *data, std::size_t size) : data_(data), size_(size) {}
  span_view(const std::vector<T> &vec) : data_(vec.data()), size_(vec.size()) {}

  const T *begin() const { return data_; }
  const T *end() const { return data_ + size_; }
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  const T &operator[](std::size_t idx) const { return data_[idx]; }

private:
  const T *data_;
  std::size_t size_;
};
#define QUANT_SPAN span_view
#endif

namespace quant {

enum class Compounding {
  Annual = 1,
  Semi = 2,
  Quarterly = 4,
  Monthly = 12,
  Continuous = 0
};

struct ZeroQuote {
  double time;
  double df;
}; // time in years, discount factor

class DiscountCurve {
public:
  // Flat constructor
  DiscountCurve(double flatYield, Compounding cmp, DayCount dc);

  // Boot-strapped constructor
  DiscountCurve(QUANT_SPAN<const ZeroQuote> quotes);

  double df(double t) const;           // P(0,t)
  double fwdBondPrice(double t) const; // for option underlying = 1/df
private:
  double y_;
  Compounding m_;
  [[maybe_unused]] DayCount dc_;
  std::vector<ZeroQuote> boot_;
};

} // namespace quant