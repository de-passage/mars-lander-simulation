#include "simulation.hpp"

#include "constants.hpp"
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
  assert((next_tick.status == status::crash && next_tick.reason != 0) ||
         next_tick.status != status::crash);
  bool should_continue = next_tick.status == status::none;
  history_.push_back(std::move(next_tick));
  current_frame_++;
  return should_continue;
}

std::pair<simulation::status, simulation::crash_reason>
simulation::touchdown_(const coord_t &start, tick_data &next) const {
  assert(coordinates->size() > 1);
  auto &next_data = next.data;
  for (size_t i = 0; i < coordinates->size() - 1; ++i) {
    auto current_segment = segment{(*coordinates)[i], (*coordinates)[i + 1]};
    auto inter =
        intersection(current_segment, segment{start, next_data.position});
    if (inter) {
      crash_reason reason = crash_reason::none;

      if (std::abs(next_data.velocity.x) > MAX_HORIZONTAL_SPEED) {
        reason = static_cast<simulation::crash_reason>(
            reason | crash_reason::h_too_fast);
      }
      if (std::abs(next_data.velocity.y) > MAX_VERTICAL_SPEED) {
        reason = static_cast<crash_reason>(reason | crash_reason::v_too_fast);
      }
      if (next_data.rotate != 0) {
        reason = static_cast<simulation::crash_reason>(reason |
                                                       crash_reason::rotation);
      }

      if (current_segment.start.y == current_segment.end.y) {
        if (reason != crash_reason::none) {
          return {simulation::status::crash, reason};
        }

        next_data.position = *inter;
        return {simulation::status::land, crash_reason::none};
      } else {
        reason =
            static_cast<crash_reason>(reason | crash_reason::uneven_ground);
        return {simulation::status::crash, reason};
      }
    }
  }
  return {simulation::status::none, crash_reason::none};
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
    next_data.reason = crash_reason::none;
  } else {
    auto [status, reason] = touchdown_(current.position, next_data);
    next_data.status = status;
    next_data.reason = reason;
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
      last.data.position.x > landing.end.x ||
      (last.data.position.y != landing.start.y)) {
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
