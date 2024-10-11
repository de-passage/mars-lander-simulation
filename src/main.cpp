#include <SFML/Graphics.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

namespace fs = std::filesystem;

using coordinates = sf::Vector2i;
using coordinate_list = std::vector<coordinates>;

constexpr static inline int GAME_WIDTH = 7000;
constexpr static inline int GAME_HEIGHT = 3000;

constexpr static inline double MARS_GRAVITY = 3.711;        // ms^2
constexpr static inline double MAX_VERTICAL_SPEED = 40.0;   // m/s
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

  for (size_t i = 0; i < line.size(); ++i) {
    sf::Vector2f position;
    position.x =
        (static_cast<double>(line[i].x) / static_cast<double>(GAME_WIDTH)) *
        window_width;
    position.y = (1.0f - (static_cast<float>(line[i].y) /
                          static_cast<double>(GAME_HEIGHT))) *
                 window_height;
    vertices[i].position = position;
    vertices[i].color = sf::Color::Red;
  }
  return vertices;
}

struct game_data {
  fs::path resource_path{"data"};
  coordinate_list coordinates;
  std::optional<fs::path> current_file;

  sf::Vector2u view_size;
  sf::VertexArray line;
};

void draw_file_selection(game_data &data) {
  if (ImGui::Begin("File Selection")) {
    if (fs::exists(data.resource_path)) {
      auto files = fs::directory_iterator(data.resource_path);
      for (const auto &file : files) {
        if (file.is_regular_file()) {
          if (ImGui::Selectable(file.path().filename().string().c_str())) {
            data.current_file = file.path();
            data.coordinates = load_line(file.path());
            data.line = create_line(data.coordinates, data.view_size);
          }
        }
      }
    }
  }
  ImGui::End();
}

void draw_gui(game_data &data) {
  // Example ImGui window
  if (ImGui::Begin("Coordinates")) {

    if (ImGui::BeginTable("table-coordinates", 2,
                          ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed);
      for (size_t i = 0; i < data.coordinates.size(); ++i) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", data.coordinates[i].x);
        ImGui::TableNextColumn();
        ImGui::Text("%d", data.coordinates[i].y);
      }
    }
    ImGui::EndTable();
  }
  ImGui::End();

  draw_file_selection(data);
}

int main(int argc, const char *argv[]) try {

  game_data data;

  if (argc == 2 && fs::exists(argv[1])) {
    data.current_file = fs::path(argv[1]);
  }
  if (data.current_file) {
    data.coordinates = load_line(data.current_file.value());
  }

  constexpr int INIT_WIDTH = 800;
  constexpr int INIT_HEIGHT = 600;

  // Create SFML window
  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  data.view_size = window.getSize();
  window.setFramerateLimit(60);

  const auto initial_size = window.getSize();
  data.line = create_line(data.coordinates, window.getSize());

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
