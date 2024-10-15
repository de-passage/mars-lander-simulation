#pragma once

#include <cassert>
#include <chrono>
#include <functional>

#include "play.hpp"
#include "simulation_data.hpp"

template <class F>
concept DecisionProcess = requires(F f) {
  { f(std::declval<simulation_data>()) } -> std::same_as<decision>;
};

inline decision do_nothing(const simulation_data &) {
  return {
      .rotate = 0,
      .power = 0,
  };
}

struct simulation {
  constexpr simulation() = default;
  constexpr simulation(const simulation &) = delete;
  constexpr simulation(simulation &&) = delete;
  constexpr simulation &operator=(const simulation &) = delete;
  constexpr simulation &operator=(simulation &&) = delete;
  enum class status { stopped, crashed, running, landed, paused, lost };
  enum class status_change { none, land, crash, lost };

  using duration = std::chrono::nanoseconds;

  struct next_tick {
    simulation_data data;
    status_change change{status_change::none};
  };

  // void tick(duration delta);

  template <DecisionProcess F = decltype(do_nothing)>
  void set_data(simulation_data data, F &&process = do_nothing);
  void set_history_point(int index);

  bool simulate(decision this_turn);

  [[nodiscard]] status_change touchdown(const coordinates &current,
                                        const coordinates &next) const;
  [[nodiscard]] status current_status() const;
  [[nodiscard]] inline bool is_running() const {
    return status_ == status::running;
  }

  [[nodiscard]] inline int current_frame() const { return current_frame_; }
  [[nodiscard]] inline int frame_count() const { return history_.size(); }

  [[nodiscard]] inline const simulation_data &current_data() const {
    return history_[current_frame_];
  }
  [[nodiscard]] inline const simulation_data &next_data() const {
    int frame =
        std::max(current_frame_ + 1, static_cast<int>(history_.size() - 1));
    return history_[frame];
  }

  void run();
  void pause();

  coordinate_list coordinates;

  const std::vector<decision> &decisions() const { return decision_history_; }
  const std::vector<simulation_data> &history() const { return history_; }

  void on_data_change(std::function<void()> callback) {
    assert(callback != nullptr);
    on_data_change_.push_back(std::move(callback));
    if (history_.size() > 0) {
      on_data_change_.back()();
    }
  }

private:
  enum status status_ { status::stopped };
  int current_frame_{0};
  std::vector<simulation_data> history_;
  std::vector<decision> decision_history_;
  [[nodiscard]] next_tick compute_next_tick_(int from_frame,
                                             int wanted_rotation,
                                             int wanted_power) const;

  std::vector<std::function<void()>> on_data_change_{};

  void changed_() const;
};

template <DecisionProcess F>
void simulation::set_data(simulation_data new_data, F &&process) {
  history_.clear(); // must stay before compute_next_tick
  decision_history_.clear();
  current_frame_ = 0;
  status_ = simulation::status::stopped;
  history_.push_back(std::move(new_data));

  while (simulate(process(history_.back()))) {
  }

  changed_();

  assert(history_.size() >= 1);
}
