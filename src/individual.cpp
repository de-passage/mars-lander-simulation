#include "individual.hpp"
#include "constants.hpp"
#include "random.hpp"
#include "utility.hpp"

decision individual::operator()(const simulation_data &data,
                                const std::vector<coordinates> &ground_line,
                                int current_frame) const {
  auto next_pos = data.position + data.velocity;
  coordinates top_left = {
      std::min(data.position.x, next_pos.x),
      std::min(data.position.y, next_pos.y),
  };
  coordinates bottom_right = {
      std::max(data.position.x, next_pos.x),
      std::max(data.position.y, next_pos.y),
  };

  if (top_left.x <= landing_site_.end.x ||
      bottom_right.x >= landing_site_.start.x) {

    if (segments_intersect(landing_site_, {data.position, next_pos})) {
      return {.rotate = 0, .power = data.power};
    }
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
  ASSERT(result.rotate >= -MAX_ROTATION && result.rotate <= MAX_ROTATION);
  ASSERT(result.power >= 0 && result.power <= MAX_POWER);
  return result;
}

individual random_individual(const simulation_data &initial,
                             const segment<coordinates> &landing_site) {
  individual ind{landing_site};
  for (auto &gene : ind.genes) {
    gene.rotate = randf();
    gene.power = randf();
  }
  return ind;
}

individual fixed_values(const simulation_data &initial, double rotate,
                        double power,
                        const segment<coordinates> &landing_site) {
  individual ind{landing_site};
  for (auto &gene : ind.genes) {
    gene.rotate = rotate;
    gene.power = power;
  }
  return ind;
}

generation random_generation(size_t size, const simulation_data &initial,
                             const segment<coordinates> &landing_site) {
  generation gen;
  gen.reserve(size);

  gen.push_back(fixed_values(initial, 0.5, 0.5, landing_site));
  gen.push_back(fixed_values(initial, 0., 0., landing_site));
  gen.push_back(fixed_values(initial, 1., 1., landing_site));
  gen.push_back(fixed_values(initial, 1., 0., landing_site));
  gen.push_back(fixed_values(initial, 0., 1., landing_site));
  gen.push_back(fixed_values(initial, 1., .5, landing_site));
  gen.push_back(fixed_values(initial, .5, 1., landing_site));

  for (size_t i = gen.size(); i < size; ++i) {
    gen.push_back(random_individual(initial, landing_site));
  }
  return gen;
}
