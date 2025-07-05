#include "MonteCarlo.hpp"
#include "../instruments/EuropeanBondOption.hpp"
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <utility>

namespace quant {

double MonteCarlo::mcPrice(double F0, double K, double sigma, double T,
                           double df, OptionType tp, std::size_t N) {

  Config defaultConfig;
  std::mt19937 rng(defaultConfig.randomSeed);
  return simulateVectorized(F0, K, sigma, T, df, tp, N, defaultConfig, rng);
}

double MonteCarlo::mcPriceAdvanced(double F0, double K, double sigma, double T,
                                   double df, OptionType tp, std::size_t N,
                                   const Config &config) {

  std::mt19937 rng(config.randomSeed);
  return simulateVectorized(F0, K, sigma, T, df, tp, N, config, rng);
}

MonteCarlo::MCResult MonteCarlo::mcPriceWithStats(double F0, double K,
                                                  double sigma, double T,
                                                  double df, OptionType tp,
                                                  std::size_t N,
                                                  const Config &config) {

  std::mt19937 rng(config.randomSeed);

  MCResult result;
  result.effectivePaths = config.useAntithetic ? N * 2 : N;

  // Store payoffs for statistics
  std::vector<double> payoffs;
  payoffs.reserve(result.effectivePaths);

  // Precompute drift and volatility terms once
  double sqrtT = std::sqrt(T);
  double drift = -0.5 * sigma * sigma * T;

  std::normal_distribution<double> normal(0.0, 1.0);

  // Process in batches
  for (std::size_t batch = 0; batch < N; batch += config.batchSize) {
    std::size_t currentBatchSize = std::min(config.batchSize, N - batch);

    if (config.enableVectorization && currentBatchSize > 1) {
      // Vectorized processing
      Eigen::ArrayXd randoms(currentBatchSize);
      for (std::size_t i = 0; i < currentBatchSize; ++i) {
        randoms(i) = normal(rng);
      }

      if (config.useAntithetic) {
        auto [paths1, paths2] = generateAntitheticPaths(F0, sigma, T, randoms);

        for (std::size_t i = 0; i < currentBatchSize; ++i) {
          payoffs.push_back(payoff(paths1(i), K, tp));
          payoffs.push_back(payoff(paths2(i), K, tp));
        }
      } else {
        Eigen::ArrayXd paths = generatePaths(F0, sigma, T, randoms);
        for (std::size_t i = 0; i < currentBatchSize; ++i) {
          payoffs.push_back(payoff(paths(i), K, tp));
        }
      }
    } else {
      // Scalar processing for small batches
      for (std::size_t i = 0; i < currentBatchSize; ++i) {
        double Z = normal(rng);
        double FT = F0 * std::exp(drift + sigma * sqrtT * Z);
        payoffs.push_back(payoff(FT, K, tp));

        if (config.useAntithetic) {
          double FT_anti = F0 * std::exp(drift + sigma * sqrtT * (-Z));
          payoffs.push_back(payoff(FT_anti, K, tp));
        }
      }
    }
  }

  // Calculate statistics
  double sum = 0.0;
  double sumSquares = 0.0;
  for (double p : payoffs) {
    sum += p;
    sumSquares += p * p;
  }

  result.price = df * sum / payoffs.size();

  double variance = (sumSquares / payoffs.size()) -
                    (sum / payoffs.size()) * (sum / payoffs.size());
  result.standardError = df * std::sqrt(variance / payoffs.size());
  result.confidenceInterval95 = 1.96 * result.standardError;

  return result;
}

double MonteCarlo::simulateVectorized(double F0, double K, double sigma,
                                      double T, double df, OptionType tp,
                                      std::size_t N, const Config &config,
                                      std::mt19937 &rng) {

  if (T <= 0.0) {
    // Expired option
    return df * payoff(F0, K, tp);
  }

  // Precompute drift and volatility terms once
  double sqrtT = std::sqrt(T);
  double drift = -0.5 * sigma * sigma * T;

  std::normal_distribution<double> normal(0.0, 1.0);
  double payoffSum = 0.0;
  std::size_t totalPaths = 0;

  // Process in batches of 8k (or configured batch size)
  for (std::size_t batch = 0; batch < N; batch += config.batchSize) {
    std::size_t currentBatchSize = std::min(config.batchSize, N - batch);

    if (config.enableVectorization && currentBatchSize > 1) {
      // Generate random numbers for this batch
      Eigen::ArrayXd randoms(currentBatchSize);
      for (std::size_t i = 0; i < currentBatchSize; ++i) {
        randoms(i) = normal(rng);
      }

      if (config.useAntithetic) {
        // Antithetic variates: Z and -Z
        auto [paths1, paths2] = generateAntitheticPaths(F0, sigma, T, randoms);

        for (std::size_t i = 0; i < currentBatchSize; ++i) {
          payoffSum += payoff(paths1(i), K, tp);
          payoffSum += payoff(paths2(i), K, tp);
        }
        totalPaths += 2 * currentBatchSize;

      } else {
        // Standard paths
        Eigen::ArrayXd paths = generatePaths(F0, sigma, T, randoms);

        for (std::size_t i = 0; i < currentBatchSize; ++i) {
          payoffSum += payoff(paths(i), K, tp);
        }
        totalPaths += currentBatchSize;
      }
    } else {
      // Scalar processing for small batches
      for (std::size_t i = 0; i < currentBatchSize; ++i) {
        double Z = normal(rng);

        // F_T = F_0 * exp((-0.5*σ²)*T + σ*√T*Z)
        double FT = F0 * std::exp(drift + sigma * sqrtT * Z);
        payoffSum += payoff(FT, K, tp);
        totalPaths++;

        if (config.useAntithetic) {
          // Antithetic path: use -Z
          double FT_anti = F0 * std::exp(drift + sigma * sqrtT * (-Z));
          payoffSum += payoff(FT_anti, K, tp);
          totalPaths++;
        }
      }
    }
  }

  // Return discounted average payoff
  return df * (payoffSum / totalPaths);
}

double MonteCarlo::payoff(double FT, double K, OptionType tp) {
  if (tp == OptionType::Call) {
    return std::max(FT - K, 0.0);
  } else {
    return std::max(K - FT, 0.0);
  }
}

Eigen::ArrayXd MonteCarlo::generatePaths(double F0, double sigma, double T,
                                         const Eigen::ArrayXd &randoms) {

  // F_T = F_0 * exp((-0.5*σ²)*T + σ*√T*Z)
  double drift = -0.5 * sigma * sigma * T;
  double vol_sqrt_T = sigma * std::sqrt(T);

  Eigen::ArrayXd paths = F0 * (drift + vol_sqrt_T * randoms).exp();
  return paths;
}

std::pair<Eigen::ArrayXd, Eigen::ArrayXd>
MonteCarlo::generateAntitheticPaths(double F0, double sigma, double T,
                                    const Eigen::ArrayXd &randoms) {

  // F_T = F_0 * exp((-0.5*σ²)*T + σ*√T*Z)
  double drift = -0.5 * sigma * sigma * T;
  double vol_sqrt_T = sigma * std::sqrt(T);

  // Path 1: Z
  Eigen::ArrayXd paths1 = F0 * (drift + vol_sqrt_T * randoms).exp();

  // Path 2: -Z (antithetic)
  Eigen::ArrayXd paths2 = F0 * (drift + vol_sqrt_T * (-randoms)).exp();

  return std::make_pair(paths1, paths2);
}

} // namespace quant