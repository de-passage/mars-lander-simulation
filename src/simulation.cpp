#include "simulation.hpp"

#include "constants.hpp"
#include "math.hpp"
#include <cassert>
#include <cmath>

simulation::status simulation::current_status() const { return status_; }

void simulation::run() {
  if (status_ == simulation::status::paused) {
    status_ = simulation::status::running;
    current_frame_ = history_.size() - 1;
  } else if (status_ == status::stopped) {
    status_ = simulation::status::running;
    current_frame_ = 0;
  }
}
void simulation::pause() {
  if (status_ == simulation::status::running) {
    status_ = simulation::status::paused;
  }
}

/// Returns true if the simulation keeps running after this turn
/// False indicates touchdown or crash
bool simulation::simulate(decision this_turn) {
  assert(coordinates.size() > 1);
  decision_history_.push_back(this_turn);
  auto wanted_rotation =
      std::min(MAX_ROTATION, std::max(-MAX_ROTATION, this_turn.rotate));
  auto wanted_power = std::min(std::min(current_data().fuel, MAX_POWER),
                               std::max(0, this_turn.power));
  if (auto wanted_power_change = std::abs(wanted_power);
      wanted_power_change > 1) {
    wanted_power = wanted_power_change / wanted_power;
  }
  if (auto wanted_rotation_change = std::abs(wanted_rotation);
      wanted_rotation_change > MAX_TURN_RATE) {
    wanted_rotation =
        (wanted_rotation_change / wanted_rotation) * MAX_TURN_RATE;
  }

  auto next_tick =
      compute_next_tick_(current_frame_, wanted_rotation, wanted_power);
  bool should_continue = next_tick.change == status_change::none;
  if (next_tick.change == status_change::land) {
    status_ = status::landed;
  } else if (next_tick.change == status_change::crash) {
    status_ = status::crashed;
  }
  history_.push_back(std::move(next_tick.data));
  current_frame_++;
  return should_continue;
}

simulation::status_change
simulation::touchdown(const ::coordinates &start,
                      const ::coordinates &next) const {
  assert(coordinates.size() > 1);
  const auto &current = current_data();
  for (size_t i = 0; i < coordinates.size() - 1; ++i) {
    auto current_segment = segment{coordinates[i], coordinates[i + 1]};
    if (segments_intersect(segment{start, next}, current_segment)) {
      if (current_segment.start.y == current_segment.end.y) {
        if (current.velocity.x <= MAX_HORIZONTAL_SPEED &&
            current.velocity.y <= MAX_VERTICAL_SPEED) {
          return simulation::status_change::land;
        }
      }
      return simulation::status_change::crash;
    }
  }
  return simulation::status_change::none;
}

void simulation::set_history_point(int index) {
  assert(index >= 0);
  assert(index < history_.size());
  current_frame_ = index;
  pause();
  if (on_data_change_) {
    on_data_change_();
  }
}

simulation::next_tick simulation::compute_next_tick_(int from_frame,
                                                     int wanted_rotation,
                                                     int wanted_power) const {
  assert(from_frame <= history_.size() - 1);
  assert(from_frame >= 0);
  const auto &current = history_[from_frame];
  next_tick next_data;
  next_data.data.power = wanted_power;
  next_data.data.fuel = current.fuel - wanted_power;
  next_data.data.rotate =  wanted_rotation;
  next_data.data.velocity.x =
      current.velocity.x +
      wanted_power * std::cos(current.rotate * DEG_TO_RAD);
  next_data.data.velocity.y =
      current.velocity.y +
      wanted_power * std::sin(current.rotate * DEG_TO_RAD) - MARS_GRAVITY;
  next_data.data.position =
      current.position + ::coordinates{current.velocity.x, current.velocity.y};

  next_data.change = touchdown(current.position, next_data.data.position);
  return next_data;
}
