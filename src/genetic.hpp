#pragma once

#include "individual.hpp"
#include "simulation.hpp"
#include "simulation_data.hpp"
#include "threadpool.hpp"

#include <atomic>
#include <mutex>
#include <vector>

struct ga_data {
  using fitness_score = double;

  using generation_result = std::vector<simulation::result>;
  using fitness_score_list = std::vector<fitness_score>;

  struct generation_parameters {
    float mutation_rate{.02};
    float elitism_rate{.14};
    unsigned int population_size{100};

    float fuel_weight = .01;
    float vertical_speed_weight = 1.;
    float horizontal_speed_weight = .98;
    float distance_weight = 1.;
    float rotation_weight = .1;
    float elite_multiplier = 5.;
    float stdev_threshold = .1;
  };

  ga_data(coordinate_list coordinates = {}, simulation_data initial = {})
      : coordinates_{std::move(coordinates)}, initial_{std::move(initial)} {
    landing_site_ = find_landing_site_();
  }

  void simulate_initial_generation(generation_parameters params);

  void set_data(coordinate_list coordinates, simulation_data initial) {
    std::lock_guard lock{mutex_};
    coordinates_ = std::move(coordinates);
    initial_ = std::move(initial);
    landing_site_ = find_landing_site_();
    current_generation_results_.clear();
    current_generation_name_ = 0;
    current_generation_.clear();
  }

  generation_result current_generation_results() const {
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

  size_t current_generation_name() const { return current_generation_name_; }

  const std::vector<individual> &current_generation() const {
    return current_generation_;
  }

  struct fitness_values {
    fitness_score score;

    double fuel_score;
    double vertical_speed_score;
    double horizontal_speed_score;
    double dist_score;
    double rotation_score;

    double weighted_fuel_score;
    double weighted_vertical_speed_score;
    double weighted_horizontal_speed_score;
    double weighted_dist_score;
    double weighted_rotation_score;

    double distance;
  };

  static fitness_values
  compute_fitness_values(const simulation::result &result,
                         const generation_parameters &params,
                         const segment<coordinates> &landing_site);

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

  segment<coordinates> find_landing_site_() const;

  static generation_result simulate_(const generation& current_generation, const coordinate_list& coordinates, const simulation_data& initial);

  segment<coordinates> landing_site_{};

  static thread_pool tp_;
};
