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

struct ga_data {
  using fitness_score = double;

  using generation_result = std::vector<simulation::simulation_result>;
  using fitness_score_list = std::vector<fitness_score>;

  ga_data(coordinate_list coordinates = {}, simulation_data initial = {})
      : coordinates_{std::move(coordinates)}, initial_{std::move(initial)} {}

  void play(unsigned int generation_size);

  void set_data(coordinate_list coordinates, simulation_data initial) {
    coordinates_ = std::move(coordinates);
    initial_ = std::move(initial);
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

  size_t current_generation_name() { return current_generation_name_; }

private:
  mutable std::mutex mutex_;
  mutable std::atomic<bool> tainted_{true};

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
};
