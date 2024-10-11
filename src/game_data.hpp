#pragma once

#include "constants.hpp"

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

using coordinates = sf::Vector2i;
using coordinate_list = std::vector<coordinates>;

struct game_data {
  fs::path resource_path{"data"};
  coordinate_list coordinates;
  std::optional<fs::path> current_file;

  sf::Vector2u view_size;
  sf::VertexArray line;

  void updated_coordinates(const coordinate_list &new_coordinates) {
    coordinates = new_coordinates;
    sf::VertexArray vertices(sf::LineStrip, coordinates.size());

    const float window_width = static_cast<float>(view_size.x);
    const float window_height = static_cast<float>(view_size.y);

    for (size_t i = 0; i < coordinates.size(); ++i) {
      sf::Vector2f position;
      position.x =
          (static_cast<double>(coordinates[i].x) / static_cast<double>(GAME_WIDTH)) *
          window_width;
      position.y = (1.0f - (static_cast<float>(coordinates[i].y) /
                            static_cast<double>(GAME_HEIGHT))) *
                   window_height;
      vertices[i].position = position;
      vertices[i].color = sf::Color::Red;
    }
    line = std::move(vertices);
  }

};
