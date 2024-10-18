#include "genetic.hpp"
#include "individual.hpp"
#include "math.hpp"
#include "random.hpp"
#include "utility.hpp"

#include <iostream>
#include <limits>
#include <mutex>

void ga_data::simulate_initial_generation(generation_parameters params) {
  params_ = params;
  current_generation_ =
      random_generation(params.population_size, initial_, landing_site_);
  current_generation_name_ = 0;
  play_();
}

ga_data::fitness_values
ga_data::compute_fitness_values(const simulation::simulation_result &result,
                                const generation_parameters &params,
                                const segment<coordinates> &landing_site) {
  ZoneScoped;
  const auto &last = result.history.back();
  const auto square = [](auto x) { return x * x; };
  const fitness_score epsilon = std::numeric_limits<fitness_score>::epsilon();

  fitness_values values;

  fitness_score remaining_fuel = last.fuel;
  coordinates position = last.position;
  coordinates position_before_last =
      result.history[result.history.size() - 2].position;

  const double MAX_ABSOLUTE_DISTANCE =
      (double)distance(coordinates{0, 0}, coordinates{GAME_WIDTH, GAME_HEIGHT});

  values.distance = distance_to_segment(landing_site, position);
  if (segments_intersect(landing_site, {position_before_last, position})) {
    values.distance = 0;
  } else if (position.y == landing_site.start.y &&
             position.x >= landing_site.start.x &&
             position.x <= landing_site.end.x) {
    values.distance = 0;
  }
  values.dist_score =
      1. - normalize(values.distance, 0., MAX_ABSOLUTE_DISTANCE);

  values.weighted_dist_score = values.dist_score * params.distance_weight;

  const fitness_score importance_of_distance = square(values.dist_score);

  values.rotation_score =
      1. - normalize(static_cast<fitness_score>(std::abs(last.rotate)), 0.,
                     (double)MAX_ROTATION);
  if (values.distance > std::numeric_limits<double>::epsilon()) {
    values.weighted_rotation_score = 0;
  } else {
    values.weighted_rotation_score =
        values.rotation_score * params.rotation_weight * importance_of_distance;
  }

  const fitness_score importance_of_rotation = square(values.rotation_score);

  constexpr double MAX_ABSOLUTE_SPEED = 200.;

  if (std::abs(last.velocity.y) <= MAX_VERTICAL_SPEED) {
    values.vertical_speed_score = 1.;
  } else {
    const fitness_score vspeed_rel_to_max =
        std::max(0., std::abs(last.velocity.y) - MAX_VERTICAL_SPEED);
    values.vertical_speed_score =
        1. - normalize(vspeed_rel_to_max, 0., MAX_ABSOLUTE_SPEED);
  }
  if (values.distance > std::numeric_limits<double>::epsilon() ||
      last.rotate != 0) {
    values.weighted_vertical_speed_score = 0;
  } else {
    values.weighted_vertical_speed_score =
        square(values.vertical_speed_score) * params.vertical_speed_weight;
  }

  if (std::abs(last.velocity.x) <= MAX_HORIZONTAL_SPEED) {
    values.horizontal_speed_score = 1.;
  } else {
    const fitness_score hspeed_rel_to_max =
        std::max(0., std::abs(last.velocity.x) - MAX_HORIZONTAL_SPEED);
    values.horizontal_speed_score =
        1. - normalize(hspeed_rel_to_max, 0., MAX_ABSOLUTE_SPEED);
  }
  if (values.distance > std::numeric_limits<double>::epsilon() ||
      last.rotate != 0) {
    values.weighted_horizontal_speed_score = 0;
  } else {
    values.weighted_horizontal_speed_score =
        square(values.horizontal_speed_score) * params.horizontal_speed_weight;
  }

  values.fuel_score = remaining_fuel;

  if (result.final_status != simulation::status::land) {
    values.weighted_fuel_score = 0;
  } else {
    values.weighted_fuel_score = values.fuel_score * params.fuel_weight;
  }

  values.score = values.weighted_fuel_score + values.weighted_dist_score +
                 values.weighted_vertical_speed_score +
                 values.weighted_horizontal_speed_score +
                 values.weighted_rotation_score;
  return values;
}

std::pair<size_t, size_t> selection(const ga_data::fitness_score_list &scores,
                                    ga_data::fitness_score total) {
  ZoneScoped;

  // Roulette wheel selection, look into implementing the alias method. cf.
  // wikipedia and
  // https://gist.github.com/Liam0205/0b5786e9bfc73e75eb8180b5400cd1f8

  auto r1 = randf() * total;
  auto r2 = randf() * total;

  constexpr auto MAX = std::numeric_limits<size_t>::max();
  size_t p1 = MAX;
  size_t p2 = MAX;
  for (int i = 0; i < scores.size(); ++i) {
    r1 -= scores[i];
    r2 -= scores[i];
    if (p1 == MAX && r1 <= 0) {
      p1 = i;
    }
    if (p2 == MAX && r2 <= 0) {
      p2 = i;
    }
    if (p1 != MAX && p2 != MAX) {
      break;
    }
  }
  assert(p1 != MAX);
  assert(p2 != MAX);
  return {p1, p2};
}

void crossover_linear_interpolation(const individual &p1, const individual &p2,
                                    individual &child1, individual &child2) {
  ZoneScoped;

  for (int i = 0; i < child1.genes.size(); ++i) {
    auto r = randf();

    child1.genes[i].rotate =
        r * p1.genes[i].rotate + (1 - r) * p2.genes[i].rotate;
    child1.genes[i].power = r * p1.genes[i].power + (1 - r) * p2.genes[i].power;

    child2.genes[i].rotate =
        (1 - r) * p1.genes[i].rotate + r * p2.genes[i].rotate;
    child2.genes[i].power = (1 - r) * p1.genes[i].power + r * p2.genes[i].power;
  }
}

void crossover_random_selection(const individual &p1, const individual &p2,
                                individual &child1, individual &child2) {
  ZoneScoped;

  for (int i = 0; i < child1.genes.size(); ++i) {
    auto r = randf();
    if (r < .5) {
      child1.genes[i] = p1.genes[i];
      child2.genes[i] = p2.genes[i];
    } else {
      child1.genes[i] = p2.genes[i];
      child2.genes[i] = p1.genes[i];
    }
  }
}

void crossover_alternate(const individual &p1, const individual &p2,
                         individual &child1, individual &child2) {
  ZoneScoped;

  for (int i = 0; i < child1.genes.size(); ++i) {
    if (i % 2 == 0) {
      child1.genes[i] = p1.genes[i];
      child2.genes[i] = p2.genes[i];
    } else {
      child1.genes[i] = p2.genes[i];
      child2.genes[i] = p1.genes[i];
    }
  }
}

void mutate(individual &p, const ga_data::generation_parameters &params,
            double stdev) {
  ZoneScoped;
  auto mutation_rate = params.mutation_rate;
  auto threshold = params.stdev_threshold;
  if (stdev < threshold) {
    mutation_rate = params.mutation_rate * (threshold - stdev + 1) * 100;
  }

  for (auto &gene : p.genes) {
    auto r = randf();
    if (randf() < mutation_rate) {
      gene.rotate = randf();
    }
    if (randf() < mutation_rate) {
      gene.power = randf();
    }
  }
}

generation next_generation(const generation &this_generation,
                           ga_data::fitness_score_list scores,
                           const ga_data::generation_parameters &params,
                           const segment<coordinates> &landing_site) {
  using fitness_score = ga_data::fitness_score;

  generation new_generation;
  new_generation.reserve(this_generation.size());

  // Preprocessing
  fitness_score best_score = std::numeric_limits<fitness_score>::min();
  fitness_score worst_score = std::numeric_limits<fitness_score>::max();

  for (size_t i = 0; i < this_generation.size(); ++i) {
    if (scores[i] > best_score) {
      best_score = scores[i];
    }
    if (scores[i] < worst_score) {
      worst_score = scores[i];
    }
  }

  if (best_score == worst_score) {
    best_score = 1;
    worst_score = 0;
  }
  // Elitism
  // Choose a number of elites
  size_t elites =
      static_cast<size_t>(this_generation.size() * params.elitism_rate);
  std::vector<std::pair<fitness_score, size_t>> elite_indices;
  elite_indices.reserve(elites);

  // Prefill the elite indices with the first values, sorted in descending order
  for (int i = 0; i < elites; ++i) {
    elite_indices.push_back({scores[i] * params.elite_multiplier, i});
  }
  std::sort(elite_indices.begin(), elite_indices.end(),
            [](auto &a, auto &b) { return a.first > b.first; });

  // Insertion sort bound by the number of elites wanted.
  if (elites != 0) {
    for (size_t i = elites; i < scores.size(); ++i) {
      for (size_t insert = 0; insert < elite_indices.size(); ++insert) {
        auto adjusted_score = scores[i] * params.elite_multiplier;
        if (adjusted_score > elite_indices[insert].first) {
          elite_indices.insert(elite_indices.begin() + insert,
                               {adjusted_score, i});
          elite_indices.pop_back();
          break;
        }
      }
    }
    assert(elite_indices.size() == elites);
    if (elites != 0) {
      scores[0] *= 5;
    }
    DEBUG_ONLY({
      for (int i = 0; i < elite_indices.size() - 1; ++i) {
        assert(elite_indices[i].first >= elite_indices[i + 1].first);
      }
    });

    for (size_t i = 0; i < elites; ++i) {
      new_generation.push_back(this_generation[elite_indices[i].second]);
    }
  }

  // Selection
  fitness_score total = 0;
  for (auto &score : scores) {
    score = (score - worst_score) / (best_score - worst_score);
    DEBUG_ONLY(auto last = total);
    total += score;
    assert(total >= last);
  }

  int crossover_style = 0;
  double sd = standard_deviation(scores, total / scores.size());
  while (new_generation.size() < this_generation.size()) {
    ZoneScopedN("Selection, crossover and mutation");
    auto [p1, p2] = selection(scores, total);

    // Crossover
    new_generation.push_back(this_generation[p1]);
    new_generation.push_back(this_generation[p2]);
    auto &child1 = *(new_generation.end() - 2);
    auto &child2 = new_generation.back();

    crossover_style = (crossover_style + 1) % 3;

    if (crossover_style == 0) {
      crossover_linear_interpolation(this_generation[p1], this_generation[p2],
                                     child1, child2);
    } else if (crossover_style == 1) {
      crossover_random_selection(this_generation[p1], this_generation[p2],
                                 child1, child2);
    } else {
      crossover_alternate(this_generation[p1], this_generation[p2], child1,
                          child2);
    }

    // Mutation
    mutate(new_generation[new_generation.size() - 2], params, sd);

    // Drop extra child
    if (new_generation.size() >= this_generation.size()) {
      new_generation.pop_back();
    } else {
      mutate(new_generation.back(), params, sd);
    }
  }

  // Mutate the elites at the end to keep their genes during selection
  // but keep the best individual as is to prevent regression
  for (size_t i = 1; i < elites; ++i) {
    mutate(new_generation[i], params, sd);
  }

  return new_generation;
}

void ga_data::next_generation() {
  ZoneScoped;

  ga_data::fitness_score_list scores;
  scores.reserve(current_generation_results_.size());
  for (const auto &result : current_generation_results_) {
    scores.push_back(compute_fitness_values(result, params_, landing_site_).score);
  }

  auto new_generation = ::next_generation(
      current_generation_, std::move(scores), params_, landing_site_);

  assert(new_generation.size() == current_generation_.size());

  {
    std::lock_guard lock{mutex_};
    current_generation_ = std::move(new_generation);
  }
  play_();
}

segment<coordinates> ga_data::find_landing_site_() const {
  coordinates last{-1, -1};
  for (auto &coord : coordinates_) {
    if (last.x != -1 && coord.y == last.y) {
      return {last, coord};
    }
    last = coord;
  }
  return {{-1, -1}, {-1, -1}};
}

std::string to_string(simulation::status status) {
  switch (status) {
  case simulation::status::none:
    return "none";
  case simulation::status::land:
    return "land";
  case simulation::status::crash:
    return "crash";
  case simulation::status::lost:
    return "lost";
  }
  return "unknown";
}

void ga_data::play_() {
  current_generation_name_++;
  generation_result results;

  std::mutex result_mutex;
  std::condition_variable one_done;

  int done{0};

  auto size = current_generation_.size();
  results.reserve(size);

  assert(results.size() == 0);

  for (const auto& ind : current_generation_) {
    auto result = simulation::simulate(coordinates_, initial_, ind);
    {
      std::lock_guard lock{result_mutex};
      results.push_back(std::move(result));
      done++;
      one_done.notify_one();
    }
  }

  std::unique_lock wait_for_completion{result_mutex};
  one_done.wait(wait_for_completion, [&] { return done == size; });

  std::lock_guard lock{mutex_};

  current_generation_results_ = results;
  assert(current_generation_results_.size() == params_.population_size);
}
