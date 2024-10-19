#pragma once

#include "simulation.hpp"
#include "simulation_data.hpp"
#include "view_transform.hpp"

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <vector>

struct game_data {
  game_data(view_transform transform, simulation::result simu,
            coordinate_list ground_line)
      : transform{transform}, simu_{std::move(simu)}, coordinates_{std::move(
                                                          ground_line)} {}
  constexpr game_data(const game_data &) = delete;
  constexpr game_data(game_data &&) = delete;
  constexpr game_data &operator=(const game_data &) = delete;
  constexpr game_data &operator=(game_data &&) = delete;

  sf::VertexArray line;

  void update_coordinates_(coordinate_list new_coordinates);
  void set_initial_parameters_(const simulation_data &initial_data);
  void reset_simulation();

  const coordinate_list &ground_line() const { return coordinates_; }

  const view_transform transform;

  constexpr bool is_running() const { return status_ == status::running; }
  bool play();
  void stop() { status_ = status::stopped; }
  bool next_frame();

  const simulation_data &current_data() const {
    assert(simu_.history.size() > current_frame_);
    return simu_.history[current_frame_];
  }
  const simulation_data &initial_data() const {
    assert(!simu_.history.empty());
    return simu_.history.front();
  }
  const simulation_data &next_data() const {
    assert(!simu_.history.empty());
    return simu_.history[next_frame_index_()];
  }

  size_t current_frame() const { return current_frame_; }
  size_t frame_count() const { return simu_.history.size(); }

  void set_history_point(int index) {
    assert(index >= 0);
    assert(index < simu_.history.size());
    current_frame_ = index;
    data_changed_();
  }

  void on_data_change(std::function<void()> callback) {
    on_data_change_ = std::move(callback);
  }

  enum class status {
    running,
    stopped,
  };

private:
  std::function<void()> on_data_change_;
  status status_ = status::stopped;
  coordinate_list coordinates_;

  simulation_data initial_;
  simulation::result simu_;

  size_t current_frame_{0};

  size_t next_frame_index_() const {
    return std::min(current_frame_ + 1, simu_.history.size() - 1);
  }

  void data_changed_() {
    if (on_data_change_) {
      on_data_change_();
    }
  }
};
