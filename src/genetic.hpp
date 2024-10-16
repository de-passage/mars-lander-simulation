#pragma once

#include "constants.hpp"
#include "play.hpp"
#include "simulation.hpp"
#include "simulation_data.hpp"

#include <array>
#include <atomic>
#include <mutex>
#include <random>
#include <vector>

struct individual {
  struct gene {
    double rotate;
    double power;
  };
  int current_frame = 0;

  decision operator()(const simulation_data &data) {
    decision result{
        .rotate = (int)std::round(genes[current_frame].rotate * MAX_ROTATION * 2 - MAX_ROTATION),
        .power = (int)std::round(genes[current_frame].power * MAX_POWER),
    };
    current_frame = (current_frame + 1) % genes.size();
    assert(result.rotate >= -MAX_ROTATION && result.rotate <= MAX_ROTATION);
    assert(result.power >= 0 && result.power <= MAX_POWER);
    return result;
  }

  std::array<gene, 40> genes;
};

individual random_individual();

using generation = std::vector<individual>;

generation random_generation(size_t size);

struct ga_data {
  using fitness_score = double;

  using generation_result = std::vector<simulation::simulation_result>;
  using fitness_score_list = std::vector<fitness_score>;

  struct generation_parameters {
    float mutation_rate{.01};
    float elitism_rate{.1};
    unsigned int population_size{100};

    float fuel_weight = 1.;
    float vertical_speed_weight = 1.;
    float horizontal_speed_weight = 1.;
    float distance_weight = 0.01;
  };

  ga_data(coordinate_list coordinates = {}, simulation_data initial = {})
      : coordinates_{std::move(coordinates)}, initial_{std::move(initial)} {}

  void play(generation_parameters params);

  void set_data(coordinate_list coordinates, simulation_data initial) {
    coordinates_ = std::move(coordinates);
    initial_ = std::move(initial);
    current_generation_results_.clear();
    current_generation_name_ = 0;
    current_generation_.clear();
  }

  const generation_result &current_generation_results() const {
    std::lock_guard lock{mutex_};
    return current_generation_results_;
  }

  bool generated() const {
    std::lock_guard lock{mutex_};
    return !current_generation_results_.empty();
  }

  void next_generation();
  void set_params(generation_parameters params) {
    std::lock_guard lock{mutex_};
    params_ = params;
  }

  size_t current_generation_name() { return current_generation_name_; }

private:
  mutable std::mutex mutex_;
  mutable std::atomic<bool> tainted_{true};

  generation_parameters params_;
  generation current_generation_;
  coordinate_list coordinates_;
  simulation_data initial_;
  generation_result current_generation_results_;
  mutable generation_result cached_results_;
  unsigned int current_generation_name_{0};

  simulation::simulation_result play(individual &individual);
  segment<coordinates> find_landing_site_() const;
  fitness_score
  calculate_fitness_(const simulation::simulation_result &result) const;
  fitness_score_list calculate_fitness_() const;
  void play_();

  segment<coordinates> landing_site_{};
};
