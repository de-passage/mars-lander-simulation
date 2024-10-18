#include "individual.hpp"
#include "constants.hpp"
#include "random.hpp"
#include "simulation.hpp"

decision individual::operator()(const simulation_data &data,
                                const std::vector<coordinates> &ground_line,
                                int current_frame) const {
  if (segments_intersect(landing_site_,
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

static_assert(DecisionProcess<individual>,
              "individual must be a DecisionProcess");
