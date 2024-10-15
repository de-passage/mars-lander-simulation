#include "game_data.hpp"
#include "gui.hpp"
#include "load_file.hpp"
#include "coordinates_utils.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include "lander.hpp"
#include "trajectory.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

void play_simulation(game_data &game, lander &lander);

int main(int argc, const char *argv[]) try {

  constexpr int INIT_WIDTH = 700 * 1.5;
  constexpr int INIT_HEIGHT = 300 * 1.5;

  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  auto window_size = window.getSize();
  view_transform to_screen{window_size.x, window_size.y};

  game_data game{to_screen};

  if (argc == 2 && fs::exists(argv[1])) {
    game.current_file = fs::path(argv[1]);
  }

  lander lander{game, to_screen};
  lander.attach(game.simu);
  trajectory traj{to_screen};
  traj.attach(game.simu);

  if (game.current_file) {
    auto loaded = load_file(game.current_file.value());
    game.initialize(loaded);
  } else {
    auto paths = path_list(game.resource_path);
    if (!paths.empty()) {
      game.current_file = paths.front();
      auto loaded = load_file(game.current_file.value());
      game.initialize(loaded);
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

      if (event.type == sf::Event::Closed) {
        window.close();
      } else if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::C && event.key.control) {
          // Close the window
          window.close();
        }
      }
    }

    // Start new ImGui frame
    ImGui::SFML::Update(window, deltaClock.restart());
    draw_gui(game, lander);

    // Clear SFML window
    window.clear();

    if (game.current_file) {
      play_simulation(game, lander);

      window.draw(lander);
      if (game.show_trajectory) {
        window.draw(traj);
      }
      window.draw(game.line);
    }
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

  auto now = clock::now();
  if (game.is_running()) {
    auto delta = now - last_time;
    frame += delta;
    if (frame >= 1s) {
      frame -= 1s;
      if (!game.next_frame()) {
        game.stop();
      }
    }

    auto elapsed_ratio = duration_cast<duration<double>>(frame) / duration_cast<duration<double>>(1s);

    const auto &current_data = game.simu.current_data();
    const auto &next_data = game.simu.next_data();
    std::cerr << "Current power: " << current_data.power << std::endl;
    lander.update(lander::update_data{.current_position = current_data.position,
                                      .next_position = next_data.position,
                                      .current_rotation = current_data.rotate,
                                      .next_rotation = next_data.rotate,
                                      .power = current_data.power
                                      },
                  elapsed_ratio);
  } else {
    frame = 0s;
  }
  last_time = now;
}
