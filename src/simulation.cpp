#include "simulation.hpp"

#include "constants.hpp"
#include "coordinates_utils.hpp"
#include "math.hpp"
#include "utility.hpp"
#include <cassert>
#include <cmath>

/// Returns true if the simulation keeps running after this turn
/// False indicates touchdown or crash
bool simulation::simulate(decision this_turn) {
  assert(coordinates->size() > 1);
  decision_history_.push_back(this_turn);
  auto wanted_rotation =
      std::min(MAX_ROTATION, std::max(-MAX_ROTATION, this_turn.rotate));
  auto wanted_power = std::min(std::min(current_data().fuel, MAX_POWER),
                               std::max(0, this_turn.power));

  auto &current = current_data();
  if (auto wanted_power_change = wanted_power - current.power;
      std::abs(wanted_power_change) > 1) {
    wanted_power =
        current.power + (std::abs(wanted_power_change) / wanted_power_change);
  }
  wanted_power = std::min(current.fuel, wanted_power);

  if (auto wanted_rotation_change = wanted_rotation - current.rotate;
      std::abs(wanted_rotation_change) > MAX_TURN_RATE) {
    wanted_rotation =
        current.rotate +
        ((std::abs(wanted_rotation_change) / wanted_rotation_change) *
         MAX_TURN_RATE);
  }

  auto next_tick =
      compute_next_tick_(current_frame_, wanted_rotation, wanted_power);
  bool should_continue = next_tick.status == status::none;
  history_.push_back(std::move(next_tick));
  current_frame_++;
  return should_continue;
}

simulation::status simulation::touchdown_(const coord_t &start, coord_t &next,
                                          float &landing_y) const {
  assert(coordinates->size() > 1);
  const auto &current = current_data();
  for (size_t i = 0; i < coordinates->size() - 1; ++i) {
    auto current_segment = segment{(*coordinates)[i], (*coordinates)[i + 1]};
    if (segments_intersect(segment{start, next}, current_segment)) {
      if (current_segment.start.y == current_segment.end.y) {
        if (std::abs(current.velocity.x) <= MAX_HORIZONTAL_SPEED &&
            std::abs(current.velocity.y) <= MAX_VERTICAL_SPEED &&
            current.rotate == 0) {
          auto inter = intersection(current_segment, segment{start, next});
          DEBUG_ONLY({
            if (!inter) {
              std::cerr << "No intersection found between " << current_segment
                        << " and " << segment{start, next} << std::endl;
            }
            assert(inter.has_value());
          });
          next = *inter;
          return simulation::status::land;
        }
      }
      return simulation::status::crash;
    }
  }
  return simulation::status::none;
}

segment<coordinates> simulation::landing_area() const {
  assert(coordinates->size() > 1);
  for (size_t i = 0; i < coordinates->size() - 1; ++i) {
    if ((*coordinates)[i].y == (*coordinates)[i + 1].y) {
      return segment{(*coordinates)[i], (*coordinates)[i + 1]};
    }
  }
  throw std::runtime_error("No landing area found");
}

void simulation::set_history_point(int index) {
  assert(index >= 0);
  assert(index < history_.size());
  current_frame_ = index;
  changed_();
}

void simulation::changed_() const {
  for (auto &callback : on_data_change_) {
    callback();
  }
}

simulation::tick_data simulation::compute_next_tick_(int from_frame,
                                                     int wanted_rotation,
                                                     int wanted_power) const {
  assert(from_frame <= history_.size() - 1);
  assert(from_frame >= 0);
  const auto &current = current_data();
  tick_data next_data;
  next_data.data.power = wanted_power;
  next_data.data.fuel = current.fuel - wanted_power;
  next_data.data.rotate = wanted_rotation;
  next_data.data.velocity.x =
      current.velocity.x +
      wanted_power * (std::sin(next_data.data.rotate * DEG_TO_RAD));
  next_data.data.velocity.y =
      current.velocity.y +
      wanted_power * (std::cos(next_data.data.rotate * DEG_TO_RAD)) -
      MARS_GRAVITY;
  next_data.data.position = current.position + current.velocity;

  if (next_data.data.position.y < 0 ||
      next_data.data.position.y > GAME_HEIGHT ||
      next_data.data.position.x < 0 || next_data.data.position.x > GAME_WIDTH) {
    next_data.status = status::lost;
  } else {
    next_data.status = touchdown_(current.position, next_data.data.position,
                                  next_data.data.position.y);
  }
  return next_data;
}

simulation::crash_reason simulation::why_crash() const {
  auto &last = history_.back();
  auto landing = landing_area();
  if (last.status != status::crash) {
    return static_cast<crash_reason>(0);
  }
  int reason = 0;


  if (last.data.position.x < landing.start.x ||
      last.data.position.x > landing.end.x) {
    reason |= crash_reason::uneven_ground;
  }

  if (std::abs(last.data.velocity.y) > MAX_VERTICAL_SPEED) {
    reason |= crash_reason::v_too_fast;
  }

  if (std::abs(last.data.velocity.x) > MAX_HORIZONTAL_SPEED) {
    reason |= crash_reason::h_too_fast;
  }

  if (last.data.rotate != 0) {
    reason |= crash_reason::rotation;
  }

  return static_cast<crash_reason>(reason);
}
