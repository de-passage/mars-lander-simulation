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
                                           const coordinate_list &coordinates) {
  ZoneScoped;
  assert(coordinates.size() > 1);
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
      compute_next_tick(last_data, coordinates, wanted_rotation, wanted_power);
  assert((next_tick.status == status::crash && next_tick.reason != 0) ||
         next_tick.status != status::crash);
  return next_tick;
}

std::pair<simulation::status, simulation::crash_reason>
simulation::touchdown(const coordinate_list &coordinates, const coord_t &start,
                      tick_data &next) {
  ZoneScoped;
  auto &next_data = next.data;
  for (size_t i = 0; i < coordinates.size() - 1; ++i) {
    auto current_segment = segment{(coordinates)[i], (coordinates)[i + 1]};
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

simulation::tick_data
simulation::compute_next_tick(const simulation_data &current,
                              const coordinate_list &coordinates,
                              int wanted_rotation, int wanted_power) {
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
    auto [status, reason] = touchdown(coordinates, current.position, next_data);
    next_data.status = status;
    next_data.reason = reason;
  }
  return next_data;
}
