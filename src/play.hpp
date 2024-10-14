#pragma once

#include "simulation_data.hpp"
struct decision {
  int rotate;
  int power;
};

decision decide(const simulation_data& data);
