#pragma once

#include <random>

struct random_float {

  random_float() = default;

  std::random_device rd;
  std::mt19937 gen{rd()};

  double operator()() { return operator()(0., 1.); }
  double operator()(double min, double max) {
    std::uniform_real_distribution<double> dis{min, max};
    return dis(gen);
  }
};
extern random_float randf;

