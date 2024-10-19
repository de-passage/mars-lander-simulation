#include "simulation.hpp"

#include "constants.hpp"
#include "math.hpp"
#include "tracy_shim.hpp"
#include "utility.hpp"

#include <cassert>
#include <cmath>

/// Returns true if the simulation keeps running after this turn
/// False indicates touchdown or crash
simulation::tick_data simulation::simulate(const simulation_data &last_data,
                                           decision this_turn,
                                           const input_data &input) {
  ZoneScoped;
  ASSERT(input.coords.size() > 1);
  auto wanted_rotation =
      std::min(MAX_ROTATION, std::max(-MAX_ROTATION, this_turn.rotate));
  auto wanted_power = std::min(std::min(last_data.fuel, MAX_POWER),
                               std::max(0, this_turn.power));

  auto current = last_data;
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
      compute_next_tick(last_data, input, wanted_rotation, wanted_power);
  ASSERT((next_tick.status == status::crash && next_tick.reason != 0) ||
         next_tick.status != status::crash);
  return next_tick;
}

std::pair<simulation::status, simulation::crash_reason>
simulation::touchdown(const input_data &input, const coord_t &start,
                      simulation_data &next) {
  ZoneScoped;
  if (next.position.y > input.y_cutoff) {
    return {status::none, crash_reason::none};
  }

  for (size_t i = 0; i < input.coords.size() - 1; ++i) {
    auto current_segment = segment{(input.coords)[i], (input.coords)[i + 1]};
    auto inter = intersection(current_segment, segment{start, next.position});
    if (inter) {
      crash_reason reason = crash_reason::none;

      if (std::abs(next.velocity.x) > MAX_HORIZONTAL_SPEED) {
        reason = static_cast<crash_reason>(reason | crash_reason::h_too_fast);
      }
      if (std::abs(next.velocity.y) > MAX_VERTICAL_SPEED) {
        reason = static_cast<crash_reason>(reason | crash_reason::v_too_fast);
      }
      if (next.rotate != 0) {
        reason = static_cast<crash_reason>(reason | crash_reason::rotation);
      }

      if (current_segment.start.y == current_segment.end.y) {
        if (reason != crash_reason::none) {
          return {status::crash, reason};
        }

        next.position = *inter;
        return {status::land, crash_reason::none};
      } else {
        reason =
            static_cast<crash_reason>(reason | crash_reason::uneven_ground);
        return {status::crash, reason};
      }
    }
  }
  return {status::none, crash_reason::none};
}

simulation::tick_data
simulation::compute_next_tick(const simulation_data &current,
                              const input_data &input, int wanted_rotation,
                              int wanted_power) {
  ZoneScoped;
  tick_data next_data;
  next_data.data.power = wanted_power;
  next_data.data.fuel = current.fuel - wanted_power;
  next_data.data.rotate = wanted_rotation;
  next_data.data.velocity.x =
      current.velocity.x +
      wanted_power * (-std::sin(next_data.data.rotate * DEG_TO_RAD));
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
    auto [status, reason] = touchdown(input, current.position, next_data.data);
    next_data.status = status;
    next_data.reason = reason;
  }
  return next_data;
}
