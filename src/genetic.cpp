#include "genetic.hpp"
#include "constants.hpp"
#include "individual.hpp"
#include "math.hpp"
#include "random.hpp"
#include "utility.hpp"

#include <future>
#include <limits>
#include <mutex>

simulation::input_data ga_data::initial_data_() const {
  return {
      .y_cutoff = y_cutoff_, .coords = coordinates_, .initial_data = initial_};
}

void ga_data::simulate_initial_generation(generation_parameters params) {
  params_ = params;
  current_generation_ =
      random_generation(params.population_size, initial_, landing_site_);
  current_generation_name_ = 1;
  current_generation_results_ = simulate_(current_generation_, initial_data_());
}

ga_data::fitness_values
ga_data::compute_fitness_values(const simulation::result &result,
                                const generation_parameters &params,
                                const segment<coordinates> &landing_site) {
  ZoneScoped;
  const auto &last = result.history.back();
  const auto square = [](auto x) { return x * x; };
  const fitness_score epsilon = std::numeric_limits<fitness_score>::epsilon();

  fitness_values values{
      .score = 0.,
      .fuel_score = 0.,
      .vertical_speed_score = 0.,
      .horizontal_speed_score = 0.,
      .dist_score = 0.,
      .rotation_score = 0.,
      .weighted_fuel_score = 0.,
      .weighted_vertical_speed_score = 0.,
      .weighted_horizontal_speed_score = 0.,
      .weighted_dist_score = 0.,
      .weighted_rotation_score = 0.,
      .distance = 0.,
      .distance_x = 0.,
      .distance_y = 0.,
  };

  fitness_score remaining_fuel = last.fuel;
  coordinates position = last.position;
  coordinates position_before_last =
      result.history[result.history.size() - 2].position;

  const double MAX_ABSOLUTE_DISTANCE =
      (double)distance(coordinates{0, 0}, coordinates{GAME_WIDTH, GAME_HEIGHT});

  values.distance_x =
      position.x < landing_site.start.x ? landing_site.start.x - position.x
      : position.x > landing_site.end.x ? position.x - landing_site.end.x
                                        : 0;
  values.distance_y =
      position.x < landing_site.start.x ? landing_site.start.x - position.x
      : position.x > landing_site.end.x ? position.x - landing_site.end.x
                                        : 0;
  values.distance = distance_to_segment(landing_site, position);

  values.dist_score =
      GAME_WIDTH - values.distance_x;// + (GAME_HEIGHT - values.distance_y) / 100;

  if (values.distance < epsilon) {
    values.vertical_speed_score =
        std::clamp(100. - std::abs(last.velocity.y), 0., 100.);
    values.horizontal_speed_score =
        std::clamp(100. - std::abs(last.velocity.x), 0., 100.);
  }

  if (result.final_status == simulation::status::crash_on_landing_area) {
    values.rotation_score = std::clamp(90. - std::abs(last.rotate), 0., 90.);
  }

  if (result.final_status == simulation::status::land) {
    values.fuel_score = remaining_fuel;
  }

  values.score = values.dist_score + values.vertical_speed_score +
                 values.horizontal_speed_score + values.rotation_score +
                 values.fuel_score;
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
  ASSERT(p1 != MAX);
  ASSERT(p2 != MAX);
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

generation next_generation(
    const generation &this_generation,
    const std::vector<simulation::result> &current_generation_results,
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

    int landed = 0;
    int crashed_on_landing_area = 0;
    for (size_t i = 0; i < elite_indices.size(); ++i) {
      if (current_generation_results[elite_indices[i].second].final_status ==
          simulation::status::land) {
        landed++;
      } else if (current_generation_results[elite_indices[i].second]
                     .final_status ==
                 simulation::status::crash_on_landing_area) {
        crashed_on_landing_area++;
      }
    }
    if (landed > 0) {
      for (size_t i = landed; i < elite_indices.size(); ++i) {
        elite_indices[i] = elite_indices[(i - landed) % landed];
      }
    } else if (crashed_on_landing_area > 0) {
      for (size_t i = crashed_on_landing_area; i < elite_indices.size(); ++i) {
        elite_indices[i] = elite_indices[(i - crashed_on_landing_area) %
                                         crashed_on_landing_area];
      }
    }

    ASSERT(elite_indices.size() == elites);
    /* DEBUG_ONLY({
      for (int i = 0; i < elite_indices.size() - 1; ++i) {
        ASSERT(elite_indices[i].first >= elite_indices[i + 1].first);
      }
    }); */

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
    ASSERT(total >= last);
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
    scores.push_back(
        compute_fitness_values(result, params_, landing_site_).score);
  }

  auto new_generation =
      ::next_generation(current_generation_, current_generation_results_,
                        std::move(scores), params_, landing_site_);

  ASSERT(new_generation.size() == current_generation_.size());

  {
    std::lock_guard lock{mutex_};
    current_generation_ = std::move(new_generation);
  }
  current_generation_name_++;

  current_generation_results_ = simulate_(current_generation_, initial_data_());
}

void ga_data::prepare_initial_data_() {
  coordinates last{-1, -1};
  y_cutoff_ = std::numeric_limits<double>::min();
  for (auto &coord : coordinates_) {
    if (last.x != -1 && coord.y == last.y) {
      landing_site_ = {last, coord};
    }
    if (coord.y > y_cutoff_) {
      y_cutoff_ = coord.y;
    }
    last = coord;
  }
}

thread_pool ga_data::tp_{};

ga_data::generation_result
ga_data::simulate_(const generation &current_generation,
                   const simulation::input_data &input) {
  generation_result results;
  using sim_results = simulation::result;
  std::vector<std::future<sim_results>> futures;

  auto size = current_generation.size();
  results.reserve(size);
  futures.reserve(size);

  for (const auto &ind : current_generation) {
    std::packaged_task<sim_results()> task(
        [&input, individual = std::move(ind)]() {
          return simulation::simulate(input, individual);
        });

    futures.push_back(task.get_future());
    tp_.push(std::move(task));
  }

  for (auto &future : futures) {
    results.push_back(future.get());
  }

  return results;
}
