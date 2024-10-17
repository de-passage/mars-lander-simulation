#include "individual.hpp"
#include "simulation.hpp"
#include "constants.hpp"

decision individual::operator()(const simulation_data &data,
                                const std::vector<coordinates> &ground_line) {
  if (landing_site_.start.x == -1) {
    find_landing_site_(ground_line);
  } else if (segments_intersect(
                 landing_site_,
                 {data.position, data.position + data.velocity})) {
    return {.rotate = 0, .power = data.power};
  }

  auto current_position = data.position;
  auto next_position = data.position + data.velocity;

  auto new_rotation = data.rotate +
                      genes[current_frame].rotate * MAX_TURN_RATE * 2 -
                      MAX_TURN_RATE;
  auto new_power = std::floor(data.power + genes[current_frame].power * 3) - 1;

  decision result{
      .rotate = std::clamp((int)std::round(new_rotation), -MAX_ROTATION,
                           MAX_ROTATION),
      .power = std::clamp((int)new_power, 0, MAX_POWER),
  };
  current_frame = (current_frame + 1) % genes.size();
  assert(result.rotate >= -MAX_ROTATION && result.rotate <= MAX_ROTATION);
  assert(result.power >= 0 && result.power <= MAX_POWER);
  return result;
}

void individual::find_landing_site_(
    const std::vector<coordinates> &ground_line) {
  coordinates last{-1, -1};
  for (auto &coord : ground_line) {
    if (last.x != -1 && coord.y == last.y) {
      landing_site_ = {last, coord};
      return;
    }
    last = coord;
  }
  throw std::runtime_error("No landing site found");
}

