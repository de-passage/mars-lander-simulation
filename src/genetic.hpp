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

  decision operator()(const simulation_data &data,
                      const std::vector<coordinates> &ground_line) {
    if (landing_site_.start.x == -1) {
      find_landing_site_(ground_line);
    } else if (segments_intersect(
                   landing_site_,
                   {data.position, data.position + data.velocity})) {
      return {.rotate = 0, .power = data.power};
    }

    auto current_position = data.position;
    auto next_position = data.position + data.velocity;

    auto new_rotation = data.rotate +
                        genes[current_frame].rotate * MAX_TURN_RATE * 2 -
                        MAX_TURN_RATE;
    auto new_power =
        std::floor(data.power + genes[current_frame].power * 3) - 1;

    decision result{
        .rotate = std::clamp((int)std::round(new_rotation), -MAX_ROTATION,
                             MAX_ROTATION),
        .power = std::clamp((int)new_power, 0, MAX_POWER),
    };
    current_frame = (current_frame + 1) % genes.size();
    assert(result.rotate >= -MAX_ROTATION && result.rotate <= MAX_ROTATION);
    assert(result.power >= 0 && result.power <= MAX_POWER);
    return result;
  }

  segment<coordinates> landing_site_{{-1, -1}, {-1, -1}};
  void find_landing_site_(const std::vector<coordinates> &ground_line) {
    coordinates last{-1, -1};
    for (auto &coord : ground_line) {
      if (last.x != -1 && coord.y == last.y) {
        landing_site_ = {last, coord};
        return;
      }
      last = coord;
    }
    throw std::runtime_error("No landing site found");
  }

  std::array<gene, 200> genes;
};
static_assert(DecisionProcess<individual>,
              "individual must be a DecisionProcess");

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

    float fuel_weight = .01;
    float vertical_speed_weight = .4;
    float horizontal_speed_weight = .4;
    float distance_weight = 1.;
    float rotation_weight = 1.;
  };

  ga_data(coordinate_list coordinates = {}, simulation_data initial = {})
      : coordinates_{std::move(coordinates)}, initial_{std::move(initial)} {}

  void play(generation_parameters params);

  void set_data(coordinate_list coordinates, simulation_data initial) {
    std::lock_guard lock{mutex_};
    coordinates_ = std::move(coordinates);
    initial_ = std::move(initial);
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
  fitness_values
  calculate_fitness(const simulation::simulation_result &result) const;
  static fitness_values
  compute_fitness_values(const simulation::simulation_result &result,
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

  simulation::simulation_result play(individual &individual);
  segment<coordinates> find_landing_site_() const;
  fitness_score
  calculate_fitness_score_(const simulation::simulation_result &result) const;
  fitness_score_list calculate_fitness_() const;
  void play_();

  segment<coordinates> landing_site_{};
};
