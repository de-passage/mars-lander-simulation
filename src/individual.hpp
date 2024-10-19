#pragma once

#include "play.hpp"
#include "simulation.hpp"
#include "simulation_data.hpp"

#include <array>

struct individual {
  struct gene {
    double rotate;
    double power;

    friend bool operator==(const gene &lhs, const gene &rhs) {
      return lhs.rotate == rhs.rotate && lhs.power == rhs.power;
    }
  };

  individual(const segment<coordinates> &landing_site): landing_site_(landing_site) {
  }

  individual(const individual &other) = default;
  individual(individual &&other) = default;

  individual &operator=(individual &&other) = default;
  individual &operator=(const individual &other) = default;

  decision operator()(const simulation_data &data,
                      const std::vector<coordinates> &ground_line,
                      int current_frame) const;

  void find_landing_site_(const std::vector<coordinates> &ground_line);

  std::array<gene, 200> genes;

  friend bool operator==(const individual &lhs, const individual &rhs) {
    return lhs.genes == rhs.genes && lhs.landing_site_ == rhs.landing_site_;
  }

private:
  segment<coordinates> landing_site_{{-1, -1}, {-1, -1}};
};
static_assert(DecisionProcess<individual>,
              "individual must be a DecisionProcess");

using generation = std::vector<individual>;

generation random_generation(size_t size, const simulation_data &initial,
                             const segment<coordinates> &landing_site);
