# QuantPricer - Bond & European Option Pricing Library

A modern C++20 quantitative finance library focused on **bond pricing** and **European bond option valuation** with clean, extensible architecture and comprehensive testing.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## ✨ Features

### 🏛️ **Core Foundation**
- **Modern Date arithmetic** with multiple day count conventions (ACT/365, ACT/360, 30/360)
- **Flexible discount curves** with flat and interpolated rate structures
- **Robust numerical solvers** for yield-to-maturity and implicit equations

### 💰 **Bond Pricing**
- **Fixed-rate bonds** with customizable coupon frequencies
- **Zero-coupon bonds** for discount instruments
- **Clean/dirty price calculations** with accrued interest
- **Risk metrics**: Modified duration, convexity, DV01
- **Multiple bond types**: Treasury, Corporate with different conventions

### 📈 **European Bond Options**
- **Black-76 model** for analytical option pricing
- **Monte Carlo simulation** with variance reduction techniques
- **Complete Greeks**: Delta, Gamma, Theta, Vega, Rho
- **Intrinsic vs. time value** decomposition

### 🔧 **Advanced Engines**
- **Sensitivity analysis** with finite difference methods
- **Yield solvers** using Newton-Raphson, Bisection, and Brent methods
- **Monte Carlo framework** with antithetic variates and control variates
- **Numerical Greeks** for complex instruments

## 🚀 Quick Start

### Prerequisites
- **C++20** compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- **CMake 3.20+**
- **Catch2** (optional, for tests)

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/quant_pricer.git
cd quant_pricer

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run tests (if Catch2 is available)
ctest
```

### Basic Usage

```cpp
#include "instruments/Bond.hpp"
#include "instruments/EuropeanBondOption.hpp"
#include "core/DiscountCurve.hpp"
#include "engines/Black76.hpp"

using namespace quant;

int main() {
    // Create a 5% Treasury bond
    auto bond = bonds::createTreasuryBond(0.05, Date(2024, 1, 1), Date(2026, 1, 1));
    
    // Set up 4% flat discount curve
    DiscountCurve curve(0.04);
    Date valuationDate(2024, 1, 1);
    
    // Price the bond
    double cleanPrice = bond.cleanPrice(curve, valuationDate);
    double duration = bond.modifiedDuration(curve, valuationDate);
    
    std::cout << "Bond Clean Price: " << cleanPrice << std::endl;
    std::cout << "Modified Duration: " << duration << " years" << std::endl;
    
    // Create and price a European call option
    EuropeanBondOption option(bond, 102.0, Date(2024, 7, 1), OptionType::CALL);
    double optionPrice = option.priceBlack76(curve, 0.15, valuationDate);
    
    std::cout << "Option Price: " << optionPrice << std::endl;
    
    return 0;
}
```

## 📚 API Documentation

### Core Components

#### **Date & Day Count**
```cpp
Date startDate(2024, 1, 1);
Date endDate(2024, 12, 31);

// Calculate year fractions with different conventions
double yf_365 = yearFraction(startDate, endDate, DayCount::ACT_365);  // Treasuries
double yf_360 = yearFraction(startDate, endDate, DayCount::ACT_360);  // Money markets
double yf_30360 = yearFraction(startDate, endDate, DayCount::THIRTY_360);  // Corporate
```

#### **Discount Curves**
```cpp
// Flat 5% curve
DiscountCurve flatCurve(0.05);

// Interpolated curve
std::vector<Date> dates = {Date(2024,6,1), Date(2025,1,1), Date(2026,1,1)};
std::vector<double> rates = {0.04, 0.045, 0.05};
DiscountCurve interpolatedCurve(dates, rates);

// Get discount factors and rates
double df = flatCurve.discountFactor(valueDate, futureDate);
double zeroRate = flatCurve.zeroRate(valueDate, futureDate);
```

### Bond Instruments

#### **Factory Functions**
```cpp
// Treasury bond (semi-annual, ACT/365)
auto treasury = bonds::createTreasuryBond(0.0375, issueDate, maturityDate);

// Corporate bond (semi-annual, 30/360) 
auto corporate = bonds::createCorporateBond(0.055, issueDate, maturityDate);

// Zero-coupon bond
auto zero = bonds::createZeroCouponBond(issueDate, maturityDate);
```

#### **Pricing & Analytics**
```cpp
// Basic pricing
double cleanPrice = bond.cleanPrice(curve, settlementDate);
double dirtyPrice = bond.dirtyPrice(curve, settlementDate);
double accrued = bond.accruedInterest(settlementDate);

// Risk metrics
double ytm = bond.yieldToMaturity(marketPrice, settlementDate);
double modDuration = bond.modifiedDuration(curve, settlementDate);
double convexity = bond.convexity(curve, settlementDate);
double dv01 = bond.dollarDuration(curve, settlementDate);
```

### European Bond Options

#### **Option Creation**
```cpp
// Manual construction
EuropeanBondOption option(bond, strikePrice, expiryDate, OptionType::CALL);

// Factory functions
auto atmCall = bond_options::createATMCall(bond, expiry, curve, valueDate);
auto otmPut = bond_options::createPut(bond, 95.0, expiry);
```

#### **Pricing Models**
```cpp
// Black-76 analytical pricing
double black76Price = option.priceBlack76(curve, volatility, valueDate);

// Monte Carlo simulation
double mcPrice = option.priceMonteCarlo(curve, volatility, valueDate, 100000);

// Greeks calculation
double delta = option.delta(curve, volatility, valueDate);
double gamma = option.gamma(curve, volatility, valueDate);
double theta = option.theta(curve, volatility, valueDate);
double vega = option.vega(curve, volatility, valueDate);
```

### Advanced Engines

#### **Numerical Solvers**
```cpp
// Yield-to-maturity calculation
auto ytmResult = YieldSolver::solveYTM(cashFlowDates, cashFlows, marketPrice, 
                                       settlementDate, dayCount);
if (ytmResult.converged) {
    std::cout << "YTM: " << ytmResult.value << std::endl;
}

// Generic root finding
auto result = YieldSolver::newtonRaphson(function, derivative, initialGuess);
```

#### **Monte Carlo Framework**
```cpp
// Configuration
MonteCarlo::Config config;
config.numSimulations = 100000;
config.useAntithetic = true;
config.randomSeed = 42;

// European option pricing
auto mcResult = MonteCarlo::priceEuropeanOption(forward, strike, expiry, 
                                                vol, df, isCall, config);

std::cout << "Price: " << mcResult.price << " ± " << mcResult.standardError << std::endl;
```

## 🏗️ Architecture

```
quant_pricer/
├── core/                    # Foundation components
│   ├── DayCount.hpp        # Date arithmetic & conventions
│   └── DiscountCurve.hpp   # Yield curve operations
├── instruments/             # Financial instruments
│   ├── Bond.hpp            # Fixed-rate bond pricing
│   └── EuropeanBondOption.hpp  # European option on bonds
├── engines/                 # Pricing & numerical engines
│   ├── YieldSolver.hpp     # Numerical root finding
│   ├── Sensitivity.hpp     # Greeks & risk calculations
│   ├── Black76.hpp         # Black-76 option model
│   └── MonteCarlo.hpp      # Monte Carlo simulation
└── tests/                   # Comprehensive test suite
    ├── bond_test.cpp       # Bond pricing tests
    └── option_test.cpp     # Option pricing tests
```

## ✅ Testing

The library includes comprehensive test suites covering:

- **Unit tests** for all core components
- **Integration tests** for pricing workflows  
- **Edge case handling** and validation
- **Numerical accuracy** verification
- **Performance benchmarks**

```bash
# Run all tests
ctest

# Run specific test suites
./bond_test
./option_test

# Run with verbose output
ctest --verbose

# Memory leak detection (Debug build)
make valgrind
```

## 🎯 Use Cases

### **Fixed Income Trading**
- Bond pricing and yield calculations
- Duration and convexity risk management
- Relative value analysis

### **Options Trading**
- European bond option valuation
- Greeks-based hedging strategies
- Volatility trading

### **Risk Management**
- Portfolio duration analysis
- Interest rate sensitivity (DV01)
- Options Greeks calculation

### **Academic Research**
- Numerical methods validation
- Monte Carlo vs. analytical pricing comparison
- Day count convention studies

## 🔬 Implementation Details

### **Numerical Accuracy**
- **IEEE 754 double precision** throughout
- **Robust convergence criteria** for iterative solvers
- **Comprehensive edge case handling**

### **Performance**
- **Header-only core** for compile-time optimization
- **Efficient memory usage** with move semantics
- **Parallel Monte Carlo** simulation support

### **Extensibility**
- **Clean interfaces** for adding new instruments
- **Pluggable pricing engines**
- **Modular design** for easy testing

## 📈 Roadmap

### **Phase 1: Core Stability** ✅
- [x] Bond pricing and analytics
- [x] European option pricing
- [x] Comprehensive testing

### **Phase 2: Extended Instruments**
- [ ] Floating rate notes
- [ ] Callable/putable bonds
- [ ] Interest rate swaps

### **Phase 3: Advanced Models**
- [ ] Binomial/trinomial trees
- [ ] Interest rate models (Vasicek, CIR)
- [ ] American option pricing

### **Phase 4: Performance**
- [ ] GPU acceleration for Monte Carlo
- [ ] Parallel curve bootstrapping
- [ ] SIMD optimization

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### **Development Setup**
```bash
# Clone with submodules
git clone --recursive https://github.com/yourusername/quant_pricer.git

# Install development dependencies
pip install pre-commit clang-format

# Set up pre-commit hooks
pre-commit install

# Format code
make format

# Generate documentation
make docs
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Black-Scholes model** foundations from Fischer Black and Myron Scholes
- **Numerical methods** inspired by "Numerical Recipes"
- **Modern C++** best practices from Bjarne Stroustrup and the Core Guidelines
- **Quantitative finance** concepts from John Hull's "Options, Futures, and Other Derivatives"

---

**Built with ❤️ for the quantitative finance community** 