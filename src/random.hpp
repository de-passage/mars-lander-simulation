#pragma once
#include "tracy_shim.hpp"

#include <cstdlib>
#include <ctime>
#include <random>

struct random_float {

  random_float() {
#ifdef FIXED_SEED
    //srand(0);
#else
    srand(time(nullptr));
#endif
  }

  std::random_device rd;
  std::mt19937 gen{rd()};

  double operator()() {
    ZoneScopedN("Random [0,1)");
    return (double)rand() / (double)(RAND_MAX);
  }

  double operator()(double min, double max) {
    ZoneScopedN("Random [min,max)");
    std::uniform_real_distribution<double> dis{min, max};
    return dis(gen);
  }
};
extern random_float randf;

