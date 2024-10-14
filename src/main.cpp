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

  constexpr int INIT_WIDTH = 800;
  constexpr int INIT_HEIGHT = 600;

  game_data data;

  if (argc == 2 && fs::exists(argv[1])) {
    data.current_file = fs::path(argv[1]);
  }

  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  data.view_size = window.getSize();

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

  lander lander{data};

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
    draw_gui(data);

    if (data.status == game_data::status::running) {
      auto elapsed = clock::now() - last_time;
      data.current.tick(elapsed);
    }
    // Clear SFML window
    window.clear();


    if (data.current_file) {
      window.draw(lander);
      window.draw(data.line);
    }
    lander.update();
    last_time = clock::now();
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
