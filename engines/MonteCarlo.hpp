#pragma once
#include <Eigen/Dense>
#include <cstddef>
#include <random>

namespace quant {

// Define option type independently to avoid circular dependency
enum class OptionType { Call, Put };

class MonteCarlo {
public:
  // Main Monte Carlo pricing function with exact signature
  static double mcPrice(double F0, double K, double sigma, double T, double df,
                        OptionType tp, std::size_t N);

  // Configuration parameters
  struct Config {
    std::size_t batchSize;    // Vectorization batch size
    bool useAntithetic;       // Enable antithetic variates
    int randomSeed;           // Fixed seed for reproducibility
    bool enableVectorization; // Use Eigen ArrayXd

    // Default constructor
    Config()
        : batchSize(8000), useAntithetic(true), randomSeed(42),
          enableVectorization(true) {}
  };

  // Advanced pricing with configuration
  static double mcPriceAdvanced(double F0, double K, double sigma, double T,
                                double df, OptionType tp, std::size_t N,
                                const Config &config = Config{});

  // Variance reduction statistics
  struct MCResult {
    double price = 0.0;
    double standardError = 0.0;
    double confidenceInterval95 = 0.0;
    std::size_t effectivePaths = 0;
    double varianceReduction = 0.0; // vs standard MC
  };

  // Full result with statistics
  static MCResult mcPriceWithStats(double F0, double K, double sigma, double T,
                                   double df, OptionType tp, std::size_t N,
                                   const Config &config = Config{});

private:
  // Core simulation engine
  static double simulateVectorized(double F0, double K, double sigma, double T,
                                   double df, OptionType tp, std::size_t N,
                                   const Config &config, std::mt19937 &rng);

  // Payoff calculation
  static double payoff(double FT, double K, OptionType tp);

  // Path generation: F_T = F_0 * exp((-0.5*σ²)*T + σ*√T*Z)
  static Eigen::ArrayXd generatePaths(double F0, double sigma, double T,
                                      const Eigen::ArrayXd &randoms);

  // Antithetic path generation
  static std::pair<Eigen::ArrayXd, Eigen::ArrayXd>
  generateAntitheticPaths(double F0, double sigma, double T,
                          const Eigen::ArrayXd &randoms);
};

} // namespace quant