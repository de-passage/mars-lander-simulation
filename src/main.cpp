#include <SFML/Graphics.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <fstream>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>

namespace fs = std::filesystem;

using coordinates = sf::Vector2i;
using coordinate_list = std::vector<coordinates>;

constexpr static inline int GAME_WIDTH = 7000;
constexpr static inline int GAME_HEIGHT = 3000;

constexpr static inline double MARS_GRAVITY = 3.711; // ms^2
constexpr static inline double MAX_VERTICAL_SPEED = 40.0; // m/s
constexpr static inline double MAX_HORIZONTAL_SPEED = 20.0; // m/s



coordinate_list load_line(const fs::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path.string());
  }

  coordinate_list line;

  int x, y;
  while (file >> x >> y) {
    line.push_back(coordinates{x, y});
  }

  return line;
}

sf::VertexArray create_line(const coordinate_list &line,
                            sf::Vector2u window_size) {
  sf::VertexArray vertices(sf::LineStrip, line.size());

  const float window_width = static_cast<float>(window_size.x);
  const float window_height = static_cast<float>(window_size.y);

  std::cout << "Recalculating line with window size: " << window_size.x << " x "
            << window_size.y << std::endl;

  for (size_t i = 0; i < line.size(); ++i) {
    sf::Vector2f position;
    position.x = (static_cast<double>(line[i].x) / static_cast<double>(GAME_WIDTH)) * window_width;
    position.y =
        (1.0f - (static_cast<float>(line[i].y) / static_cast<double>(GAME_HEIGHT))) * window_height;
    std::cout << "Line point " << i << ": " << position.x << ", " << position.y
              << std::endl;
    vertices[i].position = position;
    vertices[i].color = sf::Color::Red;
  }
  return vertices;
}

int main(int argc, const char *argv[]) try {

  if (argc < 2) {
    throw std::runtime_error("Usage: " + std::string(argv[0]) + " <path>");
  }
  if (!fs::exists(argv[1])) {
    throw std::runtime_error("File does not exist: " + std::string(argv[1]));
  }

  auto coordinates = load_line(argv[1]);

  constexpr int INIT_WIDTH = 800;
  constexpr int INIT_HEIGHT = 600;

  // Create SFML window
  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  window.setFramerateLimit(60);

  const auto initial_size = window.getSize();
  auto line = create_line(coordinates, window.getSize());

  // Initialize ImGui-SFML
  bool result = ImGui::SFML::Init(window);

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

    // Example ImGui window
    if (ImGui::Begin("Coordinates")) {

      ImGui::Text("Window size: %d x %d", window.getSize().x,
                  window.getSize().y);
      if (ImGui::BeginTable("table-coordinates", 2,
                            ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed);
        for (size_t i = 0; i < coordinates.size(); ++i) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%d", coordinates[i].x);
          ImGui::TableNextColumn();
          ImGui::Text("%d", coordinates[i].y);
        }
      }
      ImGui::EndTable();
    }
    ImGui::End();

    // Clear SFML window
    window.clear();

    // Render ImGui
    window.draw(line);
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
