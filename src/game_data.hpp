#pragma once

#include "constants.hpp"

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

using coordinates = sf::Vector2i;
using coordinate_list = std::vector<coordinates>;

struct simulation_data {
  coordinates position;
  sf::Vector2i velocity;
  int fuel;
  int rotate; //< degrees, 90 to -90
  int power;  //< 0 to 4

  using duration = std::chrono::nanoseconds;
  duration elapsed_type{0};
  void tick(duration delta) {
    using namespace std::chrono_literals;
    elapsed_type += delta;
    double ratio = delta.count() / 1000000000.;
    position += {static_cast<int>(velocity.x * ratio),
                 static_cast<int>(velocity.y * ratio)};

    if (elapsed_type >= 1s) {
      elapsed_type -= 1s;
      fuel -= power;
      velocity.y -= MARS_GRAVITY;
      velocity.x += power * std::cos(rotate * DEG_TO_RAD);
      velocity.y += power * std::sin(rotate * DEG_TO_RAD);
    }
  }
};

struct game_data {
  fs::path resource_path{"data"};
  coordinate_list coordinates;
  std::optional<fs::path> current_file;

  sf::Vector2u view_size;
  sf::VertexArray line;

  simulation_data initial;
  simulation_data current;

  enum class status { crashed, running, landed, paused } status{status::paused};

  void update_coordinates(const coordinate_list &new_coordinates);
  void set_initial_parameters(const simulation_data &initial_data);
};
