#pragma once

#include <cassert>
#include <chrono>

#include "play.hpp"
#include "simulation_data.hpp"

template <class F>
concept DecisionProcess =
    requires(F &f, const simulation_data &data,
             const std::vector<coordinates> &ground_line, int i) {
      { f(data, ground_line, i) } -> std::same_as<decision>;
    };

struct simulation {
  using coord_t = ::coordinates;
  constexpr simulation() = delete;
  constexpr simulation(const simulation &) = delete;
  constexpr simulation(simulation &&) = delete;
  constexpr simulation &operator=(const simulation &) = delete;
  constexpr simulation &operator=(simulation &&) = delete;
  enum class status { none, land, crash, lost };

  enum crash_reason {
    none = 0,
    uneven_ground = 1,
    rotation = 2,
    v_too_fast = 4,
    h_too_fast = 8,
  };

  using duration = std::chrono::nanoseconds;

  struct tick_data {
    simulation_data data;
    simulation::status status{simulation::status::none};
    simulation::crash_reason reason;
  };

  struct input_data {
    double y_cutoff;
    const std::vector<coordinates> &coords;
    const simulation_data &initial_data;
  };

  struct result {
    std::vector<simulation_data> history;
    std::vector<decision> decisions;
    simulation::status final_status;
    crash_reason reason;

    result() = default;
    result(const result &other)
        : history(other.history), decisions(other.decisions),
          final_status(other.final_status), reason(other.reason) {}
    result(result &&other) noexcept
        : history(std::move(other.history)),
          decisions(std::move(other.decisions)),
          final_status(std::move(other.final_status)),
          reason(std::move(other.reason)) {}

    result& operator=(const result &other) {
      history = other.history;
      decisions = other.decisions;
      final_status = other.final_status;
      reason = other.reason;
      return *this;
    }

    result& operator=(result &&other) noexcept {
      history = std::move(other.history);
      decisions = std::move(other.decisions);
      final_status = std::move(other.final_status);
      reason = std::move(other.reason);
      return *this;
    }

    [[nodiscard]] inline bool success() const {
      return final_status == simulation::status::land;
    }
  };
  static_assert(std::is_move_constructible_v<result>);
  static_assert(std::is_move_assignable_v<result>);

  static result simulate(const input_data &coordinates,
                         DecisionProcess auto &&process);

  static tick_data simulate(const simulation_data &last_data,
                            decision this_turn, const input_data &coordinates);

  [[nodiscard]] static tick_data
  compute_next_tick(const simulation_data &data,
                    const input_data &coordinates, int from_frame,
                    int wanted_rotation);

  [[nodiscard]] static std::pair<status, crash_reason>
  touchdown(const input_data &coordinates, const coord_t &current,
            simulation_data &next);
};

simulation::result simulation::simulate(const input_data &input,
                                        DecisionProcess auto &&process) {
  std::vector<simulation_data> history;
  history.reserve(100);
  std::vector<decision> decision_history;
  decision_history.reserve(100);

  history.push_back(std::move(input.initial_data));

  size_t current_frame = 0;
  auto last_data = input.initial_data;

  status st = status::none;
  crash_reason reason = crash_reason::none;
  while (st == status::none) {

    auto decision = process(last_data, input.coords, current_frame);
    auto tick = simulate(last_data, decision, input);

    st = tick.status;
    last_data = tick.data;
    reason = tick.reason;

    history.push_back(std::move(tick).data);
    decision_history.push_back(decision);

    current_frame++;
  }

  assert(history.size() >= 1);

  result r;
  r.history = std::move(history);
  r.decisions = std::move(decision_history);
  r.final_status = st;
  r.reason = reason;
  return r;
}
