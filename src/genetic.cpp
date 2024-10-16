#include "genetic.hpp"
#include "math.hpp"
#include "random.hpp"
#include "utility.hpp"

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

  sim.set_data(initial_, individual);
  return std::move(sim).get_simulation_result();
}

void ga_data::play(unsigned int population_size) {
  current_generation_ = random_generation(population_size);
  current_generation_name_ = 0;
  play_();
}

ga_data::fitness_score
ga_data::calculate_fitness_(const simulation::simulation_result &result) const {
  bool success = result.final_status == simulation::status::land;

  // Score:
  // - low if crashed or lost
  // - increases with remaining fuel
  // - decreases with touchdown speed if not landed
  // -> if success, remaining fuel (+ constant ?)
  // -> if failure, distance to landing site at last frame, combined with
  // remaining fuel and speed from optimal values

  const auto &last = result.history.back();

  if (success) {
    return last.data.fuel * 1000;
  } else {
    fitness_score remaining_fuel = last.data.fuel;

    segment<coordinates> landing_site = find_landing_site_();
    coordinates position = last.data.position;

    auto dist = distance_squared_to_segment(landing_site, position);

    constexpr auto fuel_weight = 100;
    constexpr auto vertical_speed_weight = 100;
    constexpr auto horizontal_speed_weight = 100;

    return remaining_fuel * fuel_weight - static_cast<fitness_score>(dist) -
           static_cast<fitness_score>(
               std::abs(MAX_VERTICAL_SPEED - last.data.velocity.y)) *
               vertical_speed_weight -
           static_cast<fitness_score>(
               std::abs(MAX_HORIZONTAL_SPEED - last.data.velocity.x)) *
               horizontal_speed_weight;
  }
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

void crossover(const individual& p1, const individual& p2, generation& new_generation) {
  new_generation.push_back(p1);
  auto& child1 = new_generation.back();
  new_generation.push_back(p2);
  auto& child2 = new_generation.back();

  for (int i = 0; i < child1.genes.size(); ++i) {
    if (randf() < 0.5) {
      std::swap(child1.genes[i], child2.genes[i]);
    }
  }
}

void mutate(individual& p) {
  for (auto& gene : p.genes) {
    if (randf() < 0.01) {
      gene.rotate = randf();
    }
    if (randf() < 0.01) {
      gene.power = randf();
    }
  }
}

void ga_data::next_generation() {
  fitness_score_list scores = calculate_fitness_();
  generation new_generation;
  new_generation.reserve(current_generation_.size());

  // Preprocessing
  fitness_score best_score = std::numeric_limits<fitness_score>::min();
  fitness_score worst_score = std::numeric_limits<fitness_score>::max();
  size_t best_index = 0;
  fitness_score total = 0;

  for (size_t i = 0; i < current_generation_.size(); ++i) {
    if (scores[i] > best_score) {
      best_score = scores[i];
      best_index = i;
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

  new_generation.push_back(current_generation_[best_index]);

  // Selection
  while (new_generation.size() < current_generation_.size()) {
    auto [p1, p2] = selection(scores, total);

    // Crossover
    crossover(current_generation_[p1], current_generation_[p2], new_generation);

    // Mutation
    mutate(new_generation[new_generation.size() - 2]);

    // Drop extra child
    if (new_generation.size() >= current_generation_.size()) {
      new_generation.pop_back();
    } else {
      mutate(new_generation.back());
    }
  }
  assert(new_generation.size() == current_generation_.size());

  current_generation_ = std::move(new_generation);
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
