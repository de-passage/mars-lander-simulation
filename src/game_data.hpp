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
};

struct simulation {
  using duration = std::chrono::nanoseconds;
  duration elapsed_time{0};
  int tick_count{0};

  void tick(duration delta);

  void set_data(simulation_data data);
  void compute_next_tick();

  enum class status { crashed, running, landed, paused } status{status::paused};

  enum class status_change { none, land, crash };
  status_change status_change_this_tick() {
    return status_change::none;
  }

  simulation_data data;
  simulation_data next_data;
  sf::Vector2f adjusted_position;
  double adjusted_rotation{0.0};
  coordinate_list coordinates;
};

struct game_data {
  fs::path resource_path{"data"};
  std::optional<fs::path> current_file;

  sf::Vector2u view_size;
  sf::VertexArray line;

  simulation_data initial;
  simulation simu;

  void update_coordinates_(coordinate_list new_coordinates);
  void set_initial_parameters_(const simulation_data &initial_data);
  void initialize(struct file_data &loaded);
  void reset_simulation();
  const coordinate_list &coordinates() const { return simu.coordinates; }
};
