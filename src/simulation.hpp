#pragma once

#include <cassert>
#include <chrono>
#include <functional>

#include "play.hpp"
#include "simulation_data.hpp"

template <class F>
concept DecisionProcess = requires(F& f, const simulation_data& data) {
  { f(data) } -> std::same_as<decision>;
};

inline decision do_nothing(const simulation_data &) {
  return {
      .rotate = 0,
      .power = 0,
  };
}

struct simulation {
  constexpr simulation(const coordinate_list &coordinates)
      : coordinates(&coordinates) {}
  constexpr simulation(const simulation &) = delete;
  constexpr simulation(simulation &&) = delete;
  constexpr simulation &operator=(const simulation &) = delete;
  constexpr simulation &operator=(simulation &&) = delete;
  enum class status { none, land, crash, lost };

  using duration = std::chrono::nanoseconds;

  struct tick_data {
    simulation_data data;
    simulation::status status{simulation::status::none};
  };

  // void tick(duration delta);

  template <DecisionProcess F = decltype(do_nothing)>
  void set_data(simulation_data data, F &&process = do_nothing);
  void set_history_point(int index);

  bool simulate(decision this_turn);

  [[nodiscard]] inline bool is_finished() const {
    return current_frame_ >= history_.size() - 1;
  }

  [[nodiscard]] status touchdown(const coordinates &current,
                                 const coordinates &next) const;

  [[nodiscard]] inline int current_frame() const { return current_frame_; }
  [[nodiscard]] inline int frame_count() const { return history_.size(); }
  [[nodiscard]] inline const tick_data &current_tick() const {
    return history_[current_frame_];
  }
  [[nodiscard]] inline const tick_data &next_tick() const {
    int frame =
        std::min(current_frame_ + 1, static_cast<int>(history_.size() - 1));
    return history_[frame];
  }

  [[nodiscard]] inline const simulation_data &current_data() const {
    return current_tick().data;
  }
  [[nodiscard]] inline const simulation_data &next_data() const {
    return next_tick().data;
  }
  [[nodiscard]] inline simulation::status current_status() const {
    return current_tick().status;
  }
  [[nodiscard]] inline simulation::status next_status() const {
    return next_tick().status;
  }

  inline bool advance_frame() {
    current_frame_++;
    if (current_frame_ >= history_.size()) {
      current_frame_ = history_.size() - 1;
      return false;
    }
    return true;
  }

  coordinate_list const *coordinates;

  const std::vector<decision> &decisions() const { return decision_history_; }
  const std::vector<tick_data> &history() const { return history_; }

  void on_data_change(std::function<void()> callback) {
    assert(callback != nullptr);
    on_data_change_.push_back(std::move(callback));
    if (history_.size() > 0) {
      on_data_change_.back()();
    }
  }

  [[nodiscard]] inline simulation::status simulation_status() const {
    assert(frame_count() > 0);
    return history_.back().status;
  }


  struct simulation_result {
    std::vector<tick_data> history;
    std::vector<decision> decisions;
    simulation::status final_status;
  };

  struct simulation_result get_simulation_result() && {
    auto status = history_.back().status;
    return {
        .history = std::move(history_),
        .decisions = std::move(decision_history_),
        .final_status = status,
    };
  }

  struct simulation_result get_simulation_result() const & {
    return {
        .history = history_,
        .decisions = decision_history_,
        .final_status = history_.back().status,
    };
  }

private:
  int current_frame_{0};
  std::vector<tick_data> history_;
  std::vector<decision> decision_history_;
  [[nodiscard]] tick_data compute_next_tick_(int from_frame,
                                             int wanted_rotation,
                                             int wanted_power) const;

  std::vector<std::function<void()>> on_data_change_{};

  void changed_() const;
};

void simulation::set_data(simulation_data new_data, DecisionProcess auto &&process) {
  history_.clear(); // must stay before compute_next_tick
  decision_history_.clear();
  history_.push_back(tick_data{std::move(new_data), status::none});
  current_frame_ = 0;

  while (simulate(process(history_.back().data))) {
  }

  // Needs to be reset since the simulation loop increases it
  current_frame_ = 0;
  changed_();

  assert(history_.size() >= 1);
}
