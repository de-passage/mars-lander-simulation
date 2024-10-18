#include "world.hpp"
#include "coordinates_utils.hpp"

#include <fstream>

void play_simulation(game_data &game, lander &lander, const config &config);

void world_data::update() {
  if (configuration.current_file && game) {
    play_simulation(*game, lander, configuration);
  }
}

void world_data::draw(sf::RenderTarget &window, sf::RenderStates states) const {
  if (configuration.current_file) {
    window.draw(lander, states);
  }

  // ga trajectories
  auto current_generation = ga.current_generation_results();
  if (selected_individual.has_value()) {
    if (configuration.show_trajectory) {
      int index = selected_individual.value();
      if (index < current_generation.size()) {
        draw_line_(current_generation[index], window, states);
      }
    }
  } else {
    for (auto &result : current_generation) {
      draw_line_(result, window, states);
    }
  }

  // ground
  sf::VertexArray line(sf::LineStrip, loaded_.ground_line.size());
  for (auto &v : loaded_.ground_line) {
    auto position = transform.to_screen(v);
    line.append(sf::Vertex{to_sfml(position), sf::Color::Red});
  }

  window.draw(line, states);
}

void world_data::draw_line_(const simulation::result &result,
                            sf::RenderTarget &window,
                            sf::RenderStates states) const {
  sf::VertexArray line(sf::LineStrip);
  sf::Color color = result.final_status == simulation::status::land
                        ? sf::Color::Cyan
                        : sf::Color::Yellow;
  for (auto &tick : result.history) {
    auto position = transform.to_screen(tick.position);
    line.append(sf::Vertex{to_sfml(position), color});
  }
  window.draw(line, states);
}

void world_data::set_file_data(file_data loaded) {
  pause_generation();
  this->loaded_ = loaded;
  for (int i = 0; i < loaded_.ground_line.size() - 1; ++i) {
    if (loaded_.ground_line[i].y == loaded_.ground_line[i + 1].y) {
      landing_site_ = {loaded_.ground_line[i], loaded_.ground_line[i + 1]};
      break;
    }
  }
  reset_individual_selection_();
  ga.set_data(loaded.ground_line, loaded.initial_values);
}

world_data::world_data(view_transform to_screen)
    : transform{to_screen}, game{}, lander{to_screen}, ga{} {}

// Utility to calculate where the lander should be in between frames
void play_simulation(game_data &game, lander &lander, const config &config) {
  using clock = std::chrono::steady_clock;
  using namespace std::chrono;
  using namespace std::chrono_literals;

  static auto last_time = clock::now();
  static clock::duration frame = 0s;
  auto playback_speed = nanoseconds(1s) / config.playback_speed;

  auto now = clock::now();
  if (game.is_running()) {
    auto delta = now - last_time;
    frame += delta;
    if (frame >= playback_speed) {
      frame -= playback_speed;
      if (!game.next_frame()) {
        game.stop();
      }
    }

    auto elapsed_ratio = duration_cast<duration<double>>(frame) /
                         duration_cast<duration<double>>(playback_speed);

    const auto &current_data = game.current_data();
    const auto &next_data = game.next_data();
    lander.update(lander::update_data{.current_position = current_data.position,
                                      .next_position = next_data.position,
                                      .current_rotation = current_data.rotate,
                                      .next_rotation = next_data.rotate,
                                      .power = current_data.power},
                  elapsed_ratio);
  } else {
    frame = 0s;
  }
  last_time = now;
}

void world_data::update_ga_params() {
  ga.set_params(ga_params);
  save_params();
}

void world_data::save_params() {
  std::ofstream file("ga_params.ini");
  file << generation_count << '\n';
  file << keep_running_after_solution << '\n';
  file << keep_running_after_max_generation << '\n';
  file << ga_params.population_size << '\n';
  file << ga_params.mutation_rate << '\n';
  file << ga_params.elitism_rate << '\n';
  file << ga_params.fuel_weight << '\n';
  file << ga_params.distance_weight << '\n';
  file << ga_params.vertical_speed_weight << '\n';
  file << ga_params.horizontal_speed_weight << '\n';
  file << ga_params.rotation_weight << '\n';
  file << ga_params.elite_multiplier << '\n';
  file << ga_params.stdev_threshold << '\n';
}

void world_data::load_params() {
  std::ifstream file("ga_params.ini");
  if (!file.is_open()) {
    return;
  }
  file >> generation_count;
  bool kr;
  file >> kr;
  keep_running_after_solution = kr;
  file >> kr;
  keep_running_after_max_generation = kr;
  file >> ga_params.population_size;
  file >> ga_params.mutation_rate;
  file >> ga_params.elitism_rate;
  file >> ga_params.fuel_weight;
  file >> ga_params.distance_weight;
  file >> ga_params.vertical_speed_weight;
  file >> ga_params.horizontal_speed_weight;
  file >> ga_params.rotation_weight;
  file >> ga_params.elite_multiplier;
  file >> ga_params.stdev_threshold;
}
