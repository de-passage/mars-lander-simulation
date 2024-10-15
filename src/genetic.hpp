#pragma once

#include "play.hpp"
#include "constants.hpp"

#include <random>
#include <vector>
#include <array>

struct individual {
  struct gene {
    double rotate;
    double power;
  };
  int current_frame = 0;

  decision operator()(const simulation_data &data) {
    decision result{
        .rotate = (int)std::round(genes[current_frame].rotate * MAX_ROTATION),
        .power = (int)std::round(genes[current_frame].power * MAX_POWER),
    };
    current_frame = (current_frame + 1) % genes.size();
    return result;
  }

  std::array<gene, 40> genes;
};

individual random_individual();

using generation = std::vector<individual>;

generation random_generation(size_t size);
