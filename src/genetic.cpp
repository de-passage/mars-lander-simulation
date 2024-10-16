#include "genetic.hpp"
#include "math.hpp"
#include "random.hpp"
#include "utility.hpp"

#include <iostream>
#include <limits>

individual random_individual() {
  individual ind;
  for (auto &gene : ind.genes) {
    gene.rotate = randf();
    gene.power = randf();
  }
  return ind;
}

generation random_generation(size_t size) {
  generation gen;
  gen.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    gen.push_back(random_individual());
  }
  return gen;
}

static_assert(DecisionProcess<individual>,
              "individual must be a DecisionProcess");

simulation::simulation_result ga_data::play(individual &individual) {
  simulation sim{coordinates_};
  landing_site_ = find_landing_site_();

  sim.set_data(initial_, individual);
  return std::move(sim).get_simulation_result();
}

void ga_data::play(generation_parameters params) {
  params_ = params;
  current_generation_ = random_generation(params.population_size);
  current_generation_name_ = 0;
  play_();
}

ga_data::fitness_score
ga_data::calculate_fitness_(const simulation::simulation_result &result) const {
  const auto &last = result.history.back();
  bool success = result.final_status == simulation::status::land;

  if (success) {
    return last.data.fuel * 1000;
  }

  fitness_score remaining_fuel = last.data.fuel;
  coordinates position = last.data.position;
  int success_multiplier = 1;

  auto dist = distance(midpoint(landing_site_), position);
  if (position.x < landing_site_.end.x && position.x > landing_site_.start.x) {
    dist = 0;
    success_multiplier++;
  }

  const auto fuel_score = remaining_fuel * params_.fuel_weight;
  const auto dist_score =
      static_cast<fitness_score>(dist) * params_.distance_weight;
  const auto vertical_speed_score =
      static_cast<fitness_score>(
          std::max(last.data.velocity.y - MAX_VERTICAL_SPEED, 0.)) *
      params_.vertical_speed_weight;
  const auto horizontal_speed_score =
      static_cast<fitness_score>(
          std::max(last.data.velocity.x - MAX_HORIZONTAL_SPEED, 0.)) *
      params_.horizontal_speed_weight;

  return (fuel_score - dist_score - vertical_speed_score -
          horizontal_speed_score) /
         success_multiplier; // probly negative
}

ga_data::fitness_score_list ga_data::calculate_fitness_() const {
  fitness_score_list scores;
  std::lock_guard lock{mutex_};
  scores.reserve(current_generation_results_.size());
  for (const auto &result : current_generation_results_) {
    scores.push_back(calculate_fitness_(result));
  }
  return scores;
}

std::pair<size_t, size_t> selection(const ga_data::fitness_score_list &scores,
                                    ga_data::fitness_score total) {

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

void crossover(const individual &p1, const individual &p2,
               generation &new_generation) {
  new_generation.push_back(p1);
  new_generation.push_back(p2);
  auto &child1 = *(new_generation.end() - 2);
  auto &child2 = new_generation.back();

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

void mutate(individual &p, const ga_data::generation_parameters &params) {
  for (auto &gene : p.genes) {
    if (randf() < params.mutation_rate) {
      gene.rotate = randf();
    }
    if (randf() < params.mutation_rate) {
      gene.power = randf();
    }
  }
}

void ga_data::next_generation() {
  fitness_score_list scores = calculate_fitness_();
  generation this_generation = [&] {
    std::lock_guard lock{mutex_};
    return current_generation_;
  }();
  generation new_generation;
  new_generation.reserve(this_generation.size());

  // Preprocessing
  fitness_score best_score = std::numeric_limits<fitness_score>::min();
  fitness_score worst_score = std::numeric_limits<fitness_score>::max();
  fitness_score total = 0;

  for (size_t i = 0; i < this_generation.size(); ++i) {
    if (scores[i] > best_score) {
      best_score = scores[i];
    }
    if (scores[i] < worst_score) {
      worst_score = scores[i];
    }
  }
  for (auto &score : scores) {
    score = (score - worst_score) / (best_score - worst_score);
    DEBUG_ONLY(auto last = total);
    total += score;
    assert(total >= last);
  }

  // Elitism
  size_t elites =
      static_cast<size_t>(this_generation.size() * params_.elitism_rate);
  std::vector<std::pair<fitness_score, size_t>> elite_indices;
  elite_indices.reserve(elites);
  for (int i = 0; i < elites; ++i) {
    elite_indices.push_back({scores[i], i});
  }
  std::sort(elite_indices.begin(), elite_indices.end(),
            [](auto &a, auto &b) { return a.first > b.first; });

  if (elites != 0) {
    for (size_t i = elites; i < scores.size(); ++i) {
      for (size_t insert = 0; insert < elite_indices.size(); ++insert) {
        if (scores[i] > elite_indices[insert].first) {
          elite_indices.insert(elite_indices.begin() + insert, {scores[i], i});
          elite_indices.pop_back();
          break;
        }
      }
    }
    assert(elite_indices.size() == elites);
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
  while (new_generation.size() < this_generation.size()) {
    auto [p1, p2] = selection(scores, total);

    // Crossover
    crossover(this_generation[p1], this_generation[p2], new_generation);

    // Mutation
    mutate(new_generation[new_generation.size() - 2], params_);

    // Drop extra child
    if (new_generation.size() >= this_generation.size()) {
      new_generation.pop_back();
    } else {
      mutate(new_generation.back(), params_);
    }
  }
  assert(new_generation.size() == this_generation.size());

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
  throw std::runtime_error("No landing site found");
}

void ga_data::play_() {
  current_generation_name_++;
  generation_result results;
  results.reserve(current_generation_.size());
  for (auto individual : current_generation_) {
    results.push_back(play(individual));
  }
  std::lock_guard lock{mutex_};
  current_generation_results_ = std::move(results);
}
