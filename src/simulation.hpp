#pragma once

#include <cassert>
#include <chrono>
#include <functional>

#include "play.hpp"
#include "simulation_data.hpp"

template <class F>
concept DecisionProcess =
    requires(F &f, const simulation_data &data,
             const std::vector<coordinates> &ground_line) {
  { f(data, ground_line) } -> std::same_as<decision>;
};

inline decision do_nothing(const simulation_data &) {
  return {
      .rotate = 0,
      .power = 0,
  };
}

struct simulation {
  using coord_t = ::coordinates;
  constexpr simulation(const coordinate_list &coordinates)
      : coordinates(&coordinates) {}
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

  crash_reason why_crash() const;
  segment<coord_t> landing_area() const;

  template <DecisionProcess F = decltype(do_nothing)>
  void set_data(simulation_data data, F &&process = do_nothing);
  void set_history_point(int index);

  bool simulate(decision this_turn);

  [[nodiscard]] inline bool is_finished() const {
    return current_frame_ >= history_.size() - 1;
  }

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
    crash_reason reason;
    crash_reason end_reason;

    [[nodiscard]] inline bool success() const {
      return final_status == simulation::status::land;
    }

    [[nodiscard]] std::optional<coord_t>
    landing_site(const segment<coord_t> &landing_site) const {
      assert(history.size() >= 2);
      auto &p1 = history[history.size() - 1].data.position;
      auto &p2 = history[history.size() - 1].data.position;
      auto i = intersection(landing_site, {p1, p2});
      assert(i.has_value());
      return i.value();
    }
  };

  struct simulation_result get_simulation_result() && {
    auto status = history_.back().status;
    auto reason = why_crash();
    assert(status != simulation::status::crash ? reason == 0 : true);
    return {
        .history = std::move(history_),
        .decisions = std::move(decision_history_),
        .final_status = status,
        .reason = reason,
    };
  }

  struct simulation_result get_simulation_result() const & {
    return {
        .history = history_,
        .decisions = decision_history_,
        .final_status = history_.back().status,
        .reason = why_crash(),
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

  [[nodiscard]] std::pair<status, crash_reason> touchdown_(const coord_t &current, coord_t &next) const;
};

void simulation::set_data(simulation_data new_data,
                          DecisionProcess auto &&process) {
  history_.clear(); // must stay before compute_next_tick
  decision_history_.clear();
  history_.push_back(tick_data{std::move(new_data), status::none});
  current_frame_ = 0;

  assert(coordinates && coordinates->size() > 1);

  while (simulate(process(history_.back().data, *coordinates))) {
  }

  // Needs to be reset since the simulation loop increases it
  current_frame_ = 0;
  changed_();

  assert(history_.size() >= 1);
}
