#pragma once

#include <cassert>
#include <chrono>
#include <functional>

#include "play.hpp"
#include "simulation_data.hpp"

template <class F>
concept DecisionProcess = requires(F &f, const simulation_data &data,
                                   const std::vector<coordinates> &ground_line,
                                   int i) {
  { f(data, ground_line, i) } -> std::same_as<decision>;
};

struct simulation {
  using coord_t = ::coordinates;
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

  struct simulation_result {
    std::vector<simulation_data> history;
    std::vector<decision> decisions;
    simulation::status final_status;
    crash_reason reason;

    [[nodiscard]] inline bool success() const {
      return final_status == simulation::status::land;
    }
  };

  static simulation_result simulate(const coordinate_list &coordinates,
                                    simulation_data data,
                                    DecisionProcess auto &&process);

  static tick_data simulate(const simulation_data &last_data,
                            decision this_turn,
                            const coordinate_list &coordinates);

  [[nodiscard]] static tick_data
  compute_next_tick(const simulation_data &data,
                    const coordinate_list &coordinates, int from_frame,
                    int wanted_rotation);

  [[nodiscard]] static std::pair<status, crash_reason>
  touchdown(const coordinate_list &coordinates, const coord_t &current,
            tick_data &next);
};

simulation::simulation_result
simulation::simulate(const coordinate_list &coordinates,
                     simulation_data new_data, DecisionProcess auto &&process) {
  std::vector<simulation_data> history;
  std::vector<decision> decision_history;

  history.push_back(std::move(new_data));

  size_t current_frame = 0;
  auto last_data = new_data;

  status st = status::none;
  crash_reason reason = crash_reason::none;
  while (st == status::none) {

    auto decision = process(last_data, coordinates, current_frame);
    auto tick = simulate(last_data, decision, coordinates);

    st = tick.status;
    last_data = tick.data;
    reason = tick.reason;

    history.push_back(std::move(tick).data);
    decision_history.push_back(decision);

    current_frame++;
  }

  assert(history.size() >= 1);

  return simulation_result{.history = std::move(history),
                           .decisions = std::move(decision_history),
                           .final_status = st,
                           .reason = reason};
}
