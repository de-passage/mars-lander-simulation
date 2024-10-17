#pragma once

#include "play.hpp"
#include "simulation_data.hpp"
#include "simulation.hpp"

#include <array>

struct individual {
  struct gene {
    double rotate;
    double power;
  };
  int current_frame = 0;

  decision operator()(const simulation_data &data,
                      const std::vector<coordinates> &ground_line);

  void find_landing_site_(const std::vector<coordinates> &ground_line);

  std::array<gene, 200> genes;
  segment<coordinates> landing_site_{{-1, -1}, {-1, -1}};
};
static_assert(DecisionProcess<individual>,
              "individual must be a DecisionProcess");

individual random_individual();

using generation = std::vector<individual>;

generation random_generation(size_t size, const simulation_data &initial);
