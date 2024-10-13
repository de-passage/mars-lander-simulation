#include "game_data.hpp"
#include "gui.hpp"
#include "load_file.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <filesystem>
#include <iostream>
#include <optional>

class lander : public sf::Drawable {

  const float lander_size = 20.f;

public:
  lander(game_data &data)
      : data{data.current}, height{data.view_size.y}, width{data.view_size.x} {
    create_shapes();
  }
  virtual void draw(sf::RenderTarget &target,
                    sf::RenderStates states) const override {
    target.draw(lander_triangle, states);
    target.draw(lander_bottom, states);
  }

  void update() {
    auto position = calculate_position();
    lander_triangle.setPosition(position);
    lander_bottom.setPosition(position.x, position.y);
    lander_triangle.setRotation(data.rotate);
    lander_bottom.setRotation(data.rotate);
  }

private:
  simulation_data &data;
  unsigned int height;
  unsigned int width;

  sf::ConvexShape lander_triangle;
  sf::RectangleShape lander_bottom;

  sf::Vector2f calculate_position() {
    const float window_width = static_cast<float>(height);
    const float window_height = static_cast<float>(width);

    sf::Vector2f position;
    position.x = (static_cast<double>(data.position.x) /
                  static_cast<double>(GAME_WIDTH)) *
                 window_width;
    position.y = (1.0f - (static_cast<float>(data.position.y) /
                          static_cast<double>(GAME_HEIGHT))) *
                 window_height;
    return position;
  }

  // Function to create a triangle indicator with a distinct bottom
  void create_shapes() {
    auto position = calculate_position();
    sf::ConvexShape triangle(3); // A triangle has 3 points
    // Define the points for the triangle (relative to the position)
    triangle.setPoint(0, sf::Vector2f(-lander_size / 2.f, 0.f)); // Left point
    triangle.setPoint(2, sf::Vector2f(lander_size / 2.f, 0.f));  // Right point
    triangle.setPoint(1, sf::Vector2f(0.f, -lander_size));       // Top point
    triangle.setFillColor(sf::Color::Green); // Fill color of the triangle
    triangle.setOrigin(0.f, 0.f);

    // Optionally, set an outline for the triangle to distinguish the bottom
    triangle.setOutlineThickness(2.f);
    triangle.setOutlineColor(
        sf::Color::Yellow); // Highlight outline (e.g., bottom side)

    lander_triangle = std::move(triangle);

    // Bottom marker
    sf::RectangleShape bottom_marker(sf::Vector2f(
        lander_size, 3.f)); // A line with width `size` and height `2`
    bottom_marker.setFillColor(
        sf::Color::Red); // Red line indicating the bottom
    bottom_marker.setOrigin(lander_size / 2.f, 0.f);
    lander_bottom = bottom_marker;
  }
};

int main(int argc, const char *argv[]) try {

  game_data data;

  if (argc == 2 && fs::exists(argv[1])) {
    data.current_file = fs::path(argv[1]);
  }
  if (data.current_file) {
    auto loaded = load_file(data.current_file.value());
    data.update_coordinates(std::move(loaded.line));
    data.set_initial_parameters(loaded.data);
  }

  constexpr int INIT_WIDTH = 800;
  constexpr int INIT_HEIGHT = 600;

  // Create SFML window
  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  data.view_size = window.getSize();
  window.setFramerateLimit(60);

  lander lander{data};

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
    draw_gui(data);

    // Clear SFML window
    window.clear();

    // Render ImGui
    window.draw(data.line);

    lander.update();
    if (data.current_file) {
      window.draw(lander);
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
