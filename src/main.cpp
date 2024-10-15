#include "coordinates_utils.hpp"
#include "game_data.hpp"
#include "gui.hpp"
#include "load_file.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include "lander.hpp"
#include "trajectory.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

void play_simulation(game_data &game, lander &lander);

template <class Data>
void handle_events(sf::RenderWindow &window, const sf::Event &event,
                   Data &world) {
  if (event.type == sf::Event::Closed) {
    window.close();
  } else if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::C && event.key.control) {
      // Close the window
      window.close();
    }
  }
}

template <class Data> void render(sf::RenderWindow &window, Data &world) {
  draw_gui(world.game);

  if (world.game.current_file) {
    play_simulation(world.game, world.lander);

    window.draw(world.lander);
    if (world.game.show_trajectory) {
      window.draw(world.traj);
    }
    window.draw(world.game.line);
  }
}

struct data {
  game_data game;
  class lander lander;
  trajectory traj;

  data(view_transform to_screen)
      : game{to_screen}, lander{game, to_screen}, traj{to_screen} {
    lander.attach(game.simu);
    traj.attach(game.simu);
  }
};

int main(int argc, const char *argv[]) try {

  constexpr int INIT_WIDTH = 700 * 1.5;
  constexpr int INIT_HEIGHT = 300 * 1.5;

  sf::RenderWindow window(sf::VideoMode::getDesktopMode(),
                          "SFML + ImGui Example");
  auto window_size = window.getSize();
  view_transform to_screen{window_size.x, window_size.y};

  data world{to_screen};

  if (argc == 2 && fs::exists(argv[1])) {
    world.game.current_file = fs::path(argv[1]);
  }

  if (world.game.current_file) {
    auto loaded = load_file(world.game.current_file.value());
    world.game.initialize(loaded);
  } else {
    auto paths = path_list(world.game.resource_path);
    if (!paths.empty()) {
      world.game.current_file = paths.front();
      auto loaded = load_file(world.game.current_file.value());
      world.game.initialize(loaded);
    }
  }

  // Create SFML window
  window.setFramerateLimit(60);

  // Initialize ImGui-SFML
  if (!ImGui::SFML::Init(window)) {
    std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
    return 1;
  }

  sf::Clock deltaClock;
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);

      handle_events(window, event, world);
    }

    // Start new ImGui frame
    ImGui::SFML::Update(window, deltaClock.restart());

    // Clear SFML window
    window.clear();
    render(window, world);
    ImGui::SFML::Render(window);

    // Display the SFML window
    window.display();
  }

  // Cleanup ImGui-SFML
  ImGui::SFML::Shutdown();

  return 0;
} catch (std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
  return 1;
}

void play_simulation(game_data &game, lander &lander) {
  using clock = std::chrono::steady_clock;
  using namespace std::chrono;
  using namespace std::chrono_literals;

  static auto last_time = clock::now();
  static clock::duration frame = 0s;
  auto playback_speed = nanoseconds(1s) / game.playback_speed;

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
