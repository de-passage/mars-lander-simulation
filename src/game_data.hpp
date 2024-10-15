#pragma once

#include "simulation.hpp"
#include "simulation_data.hpp"
#include "view_transform.hpp"

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

struct game_data {
  game_data(view_transform transform) : transform{transform} {}
  constexpr game_data(const game_data &) = delete;
  constexpr game_data(game_data &&) = delete;
  constexpr game_data &operator=(const game_data &) = delete;
  constexpr game_data &operator=(game_data &&) = delete;

  sf::VertexArray line;

  simulation_data initial;
  simulation simu;

  void update_coordinates_(coordinate_list new_coordinates);
  void set_initial_parameters_(const simulation_data &initial_data);
  void initialize(struct file_data &loaded);
  void reset_simulation();
  const coordinate_list &coordinates() const { return simu.coordinates; }

  fs::path resource_path{"data"};
  std::optional<fs::path> current_file;
  bool show_trajectory = true;
  int playback_speed = 5;
  const view_transform transform;

  constexpr bool is_running() const { return status_ == status::running; }
  bool play();
  void stop() { status_ = status::stopped; }
  bool next_frame();

  enum class status {
    running,
    stopped,
  };

private:
  status status_ = status::stopped;
};
