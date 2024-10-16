#include "world.hpp"

void play_simulation(game_data &game, lander &lander, const config &config);

void world_data::update() {
  if (configuration.current_file && game) {
      play_simulation(*game, lander, configuration);
  }
}

void world_data::draw(sf::RenderTarget &window, sf::RenderStates states) const {
  if (configuration.current_file) {
    window.draw(lander, states);
    if (game) {
      if (configuration.show_trajectory) {
        window.draw(traj, states);
      }
    }
  }

  // ga trajectories
  for (auto &result : ga.current_generation_results()) {
    sf::VertexArray line(sf::LineStrip);
    sf::Color color = result.final_status == simulation::status::land ? sf::Color::Cyan : sf::Color::Yellow;
    for (auto &tick : result.history) {
      auto position = transform.to_screen(tick.data.position);
      line.append(sf::Vertex{position, color});
    }
    window.draw(line, states);
  }

  // ground
  sf::VertexArray line(sf::LineStrip, loaded_.ground_line.size());
  for (auto& v: loaded_.ground_line) {
    auto position = transform.to_screen(v);
    line.append(sf::Vertex{position, sf::Color::Red});
  }

  window.draw(line, states);
}
void world_data::set_file_data(file_data loaded) {
  this->loaded_ = loaded;
  if (game) {
    game->initialize(loaded);
  } else {
    lander.update(loaded.initial_values.position, loaded.initial_values.rotate);
  }
  ga.set_data(loaded.ground_line, loaded.initial_values);
}

world_data::world_data(view_transform to_screen)
    : transform{to_screen}, game{}, lander{to_screen}, traj{to_screen}, ga{} {}

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

    const auto &current_data = game.simu.current_data();
    const auto &next_data = game.simu.next_data();
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