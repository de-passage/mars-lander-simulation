#include "game_data.hpp"
#include "constants.hpp"
#include "load_file.hpp"
#include "play.hpp"
#include <imgui.h>

void game_data::initialize(file_data &loaded) {
  update_coordinates_(std::move(loaded.ground_line));
  set_initial_parameters_(
      loaded.initial_values); // Must be called after update_coordinates_
  status_ = status::stopped;
}

void game_data::reset_simulation() { simu.set_data(initial, decide); }

void game_data::update_coordinates_(coordinate_list new_coordinates) {
  coordinates_ = std::move(new_coordinates);
  sf::VertexArray vertices(sf::LineStrip, coordinates_.size());

  for (size_t i = 0; i < coordinates_.size(); ++i) {
    sf::Vector2f position = transform.to_screen(coordinates_[i]);
    vertices[i].position = position;
    vertices[i].color = sf::Color::Red;
  }
  line = std::move(vertices);
}

void game_data::set_initial_parameters_(const simulation_data &initial_data) {
  initial = initial_data;
  reset_simulation();
}

bool game_data::next_frame() {
  assert(is_running());
  return simu.advance_frame();
}

bool game_data::play() {
  if (simu.is_finished()) {
    return false;
  }
  status_ = status::running;
  return true;
}
