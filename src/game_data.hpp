#pragma once

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
  duration elapsed_time{0};
  int tick_count{0};
  void tick(duration delta);
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

  void update_coordinates(coordinate_list new_coordinates);
  void set_initial_parameters(const simulation_data &initial_data);
  void initialize(struct file_data &loaded);
  void reset_simulation();
};
