#include "game_data.hpp"
#include "constants.hpp"
#include "load_file.hpp"
#include "play.hpp"
#include <imgui.h>

void game_data::reset_simulation() { set_history_point(0); }

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
  initial_ = initial_data;
  reset_simulation();
}

bool game_data::next_frame() {
  assert(is_running());
  if (current_frame_ < simu_.history.size() - 1) {
    current_frame_++;
    data_changed_();
    return true;
  }
  return false;
}

bool game_data::play() {
  if (current_frame_ >= simu_.history.size() - 1) {
    return false;
  }
  status_ = status::running;
  return true;
}



