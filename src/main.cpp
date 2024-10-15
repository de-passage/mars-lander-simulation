#include "game_data.hpp"
#include "gui.hpp"
#include "load_file.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include "lander.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

int main(int argc, const char *argv[]) try {

  constexpr int INIT_WIDTH = 700 * 1.5;
  constexpr int INIT_HEIGHT = 300 * 1.5;

  game_data data;

  if (argc == 2 && fs::exists(argv[1])) {
    data.current_file = fs::path(argv[1]);
  }

  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  data.view_size = window.getSize();


  lander lander{data};
  lander.attach(data.simu);

  if (data.current_file) {
    auto loaded = load_file(data.current_file.value());
    data.initialize(loaded);
  } else {
    auto paths = path_list(data.resource_path);
    if (!paths.empty()) {
      data.current_file = paths.front();
      auto loaded = load_file(data.current_file.value());
      data.initialize(loaded);
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
  using clock = std::chrono::steady_clock;
  auto last_time = clock::now();
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
    draw_gui(data, lander);

    // Clear SFML window
    window.clear();

    if (data.current_file) {
      auto now = clock::now();
      if (data.simu.is_running()) {
        using namespace std::chrono_literals;
        auto delta = now - last_time;
        auto elapsed_ratio = static_cast<double>((1s).count()) /
                             static_cast<double>(delta.count());

        const auto &current_data = data.simu.current_data();
        const auto &next_data = data.simu.next_data();
        lander.update(
            lander::update_data{.current_position = current_data.position,
                                .next_position = next_data.position,
                                .current_rotation = current_data.rotate,
                                .next_rotation = next_data.rotate},
            elapsed_ratio);
      }
      last_time = now;
      window.draw(lander);
      window.draw(data.line);
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
