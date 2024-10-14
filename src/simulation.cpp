#include "simulation.hpp"

#include "constants.hpp"
#include "math.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

const simulation_data &simulation::current_data() const {
  return history[current_frame_];
}
simulation::status simulation::current_status() const { return status_; }

void simulation::run() {
  if (status_ == simulation::status::paused) {
    status_ = simulation::status::running;
    current_frame_ = history.size() - 1;
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

void simulation::tick(duration delta) {
  using namespace std::chrono_literals;
  if (status_ == simulation::status::running ||
      status_ == simulation::status::paused) {
    elapsed_time += delta;
  }

  if (elapsed_time >= 1s / simulation_speed) {
    tick_count++;
    elapsed_time -= 1s;
    compute_next_tick();
  } else {
    double ratio = elapsed_time.count() / 1000000000.;
    const auto &current = current_data();

    auto distance = next_data.position - current.position;
    adjusted_position = sf::Vector2f{
        static_cast<float>(current.position.x + (distance.x * ratio / simulation_speed)),
        static_cast<float>(current.position.y + (distance.y * ratio / simulation_speed))};

    adjusted_rotation =
        current.rotate + (next_data.rotate - current.rotate) * ratio / simulation_speed;
  }
}

void simulation::simulate(decision this_turn) {
  auto wanted_rotation =
      std::min(MAX_ROTATION, std::max(-MAX_ROTATION, this_turn.rotate));
  auto wanted_power = std::min(MAX_POWER, std::max(0, this_turn.power));
  if (auto wanted_power_change = std::abs(wanted_power);
      wanted_power_change > 1) {
    wanted_power = wanted_power_change / wanted_power;
  }
  if (auto wanted_rotation_change = std::abs(wanted_rotation);
      wanted_rotation_change > MAX_TURN_RATE) {
    wanted_rotation =
        (wanted_rotation_change / wanted_rotation) * MAX_TURN_RATE;
  }

  next_data.rotate = this_turn.rotate;
  next_data.power = this_turn.power;
  compute_next_tick();
  elapsed_time = duration{0};
  tick_count = 0;
  status_ = simulation::status::running;
}

simulation::status_change simulation::touchdown(const ::coordinates &start,
                                                const ::coordinates &next) {
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

void simulation::set_data(simulation_data new_data) {
  history.clear(); // must stay before compute_next_tick
  next_data = std::move(new_data);
  compute_next_tick();
  elapsed_time = duration{0};
  tick_count = 0;
  status_ = simulation::status::stopped;
  assert(history.size() == 1);
}

void simulation::set_history_point(int index) {
  assert(index >= 0 && index < history.size());

  current_frame_ = index;
  pause();
  elapsed_time = duration{0};

  adjusted_position = {static_cast<float>(current_data().position.x),
                       static_cast<float>(current_data().position.y)};
  adjusted_rotation = current_data().rotate;
}

void simulation::compute_next_tick() {
  history.push_back(std::move(next_data));
  current_frame_ = history.size() - 1;
  const auto &current = current_data();
  next_data.fuel = current.fuel - current.power;
  next_data.velocity.x = current.velocity.x +
                         current.power * std::cos(current.rotate * DEG_TO_RAD);
  next_data.velocity.y = current.velocity.y +
                         current.power * std::sin(current.rotate * DEG_TO_RAD) -
                         MARS_GRAVITY;
  next_data.position =
      current.position +
      decltype(current.position){static_cast<int>(current.velocity.x),
                                 static_cast<int>(current.velocity.y)};

  adjusted_position = {static_cast<float>(current.position.x),
                       static_cast<float>(current.position.y)};
  adjusted_rotation = current.rotate;
}
