#pragma once

#include <random>

struct random_float {

  random_float() = default;

  std::random_device rd;
  std::mt19937 gen{rd()};
  std::uniform_real_distribution<double> dis{0.0, 1.0};

  double operator()() { return dis(gen); }
};
extern random_float randf;

