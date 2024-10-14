#pragma once

#include <chrono>

#include "play.hpp"
#include "simulation_data.hpp"

template <class F>
concept DecisionProcess = requires(F f) {
  { f(std::declval<simulation_data>()) } -> std::same_as<decision>;
};

struct simulation {
  enum class status { stopped, crashed, running, landed, paused };

public:
  enum class status_change { none, land, crash };
  using duration = std::chrono::nanoseconds;
  duration elapsed_time{0};
  int tick_count{0};

  void tick(duration delta);

  void set_data(simulation_data data);
  void set_history_point(int index);
  void compute_next_tick();

  void simulate(decision this_turn);
  void simulate(DecisionProcess auto &&decide);

  status_change touchdown(const coordinates &current, const coordinates &next);
  status current_status() const;
  inline bool is_running() const { return status_ == status::running; }

  const simulation_data &current_data() const;
  simulation_data next_data;
  sf::Vector2f adjusted_position;
  double adjusted_rotation{0.0};
  coordinate_list coordinates;

  std::vector<simulation_data> history;

  void run();
  void pause();

  int simulation_speed{2};

private:
  enum status status_ { status::stopped };
  int current_frame_{0};
};
