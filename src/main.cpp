#include "game_data.hpp"
#include "gui.hpp"
#include "load_file.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

int main(int argc, const char *argv[]) try {

  game_data data;

  if (argc == 2 && fs::exists(argv[1])) {
    data.current_file = fs::path(argv[1]);
  }
  if (data.current_file) {
    auto loaded = load_file(data.current_file.value());
    data.update_coordinates(std::move(loaded.line));
    data.initial = loaded.data;
  }

  constexpr int INIT_WIDTH = 800;
  constexpr int INIT_HEIGHT = 600;

  // Create SFML window
  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  data.view_size = window.getSize();
  window.setFramerateLimit(60);

  // Initialize ImGui-SFML
  if(!ImGui::SFML::Init(window)) {
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
    draw_gui(data);

    // Clear SFML window
    window.clear();

    // Render ImGui
    window.draw(data.line);
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
