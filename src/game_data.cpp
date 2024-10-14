#include "game_data.hpp"
#include "constants.hpp"
#include "load_file.hpp"
#include "play.hpp"
#include <imgui.h>
#include <iostream>

void game_data::initialize(file_data &loaded) {
  set_initial_parameters_(loaded.data);
  update_coordinates_(std::move(loaded.line));
}

void game_data::reset_simulation() {
  simu.set_data(initial);
  simu.tick_count = 0;
}

void game_data::update_coordinates_(coordinate_list new_coordinates) {
  simu.coordinates = std::move(new_coordinates);
  sf::VertexArray vertices(sf::LineStrip, simu.coordinates.size());

  const float window_width = static_cast<float>(view_size.x);
  const float window_height = static_cast<float>(view_size.y);

  for (size_t i = 0; i < simu.coordinates.size(); ++i) {
    sf::Vector2f position;
    position.x = (static_cast<double>(simu.coordinates[i].x) /
                  static_cast<double>(GAME_WIDTH)) *
                 window_width;
    position.y = (1.0f - (static_cast<float>(simu.coordinates[i].y) /
                          static_cast<double>(GAME_HEIGHT))) *
                 window_height;
    vertices[i].position = position;
    vertices[i].color = sf::Color::Red;
  }
  line = std::move(vertices);
}

void game_data::set_initial_parameters_(const simulation_data &initial_data) {
  initial = initial_data;
  reset_simulation();
}
