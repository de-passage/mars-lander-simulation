#include "game_data.hpp"
#include "constants.hpp"
#include "load_file.hpp"

void game_data::initialize(file_data &loaded) {
  set_initial_parameters(loaded.data);
  update_coordinates(std::move(loaded.line));
}

void game_data::reset_simulation() {
  current = initial;
  status = game_data::status::paused;
  current.tick_count = 0;
}

void game_data::update_coordinates(coordinate_list new_coordinates) {
  coordinates = std::move(new_coordinates);
  sf::VertexArray vertices(sf::LineStrip, coordinates.size());

  const float window_width = static_cast<float>(view_size.x);
  const float window_height = static_cast<float>(view_size.y);

  for (size_t i = 0; i < coordinates.size(); ++i) {
    sf::Vector2f position;
    position.x = (static_cast<double>(coordinates[i].x) /
                  static_cast<double>(GAME_WIDTH)) *
                 window_width;
    position.y = (1.0f - (static_cast<float>(coordinates[i].y) /
                          static_cast<double>(GAME_HEIGHT))) *
                 window_height;
    vertices[i].position = position;
    vertices[i].color = sf::Color::Red;
  }
  line = std::move(vertices);
}

void game_data::set_initial_parameters(const simulation_data &initial_data) {
  initial = initial_data;
  reset_simulation();
}
void simulation_data::tick(duration delta) {
  using namespace std::chrono_literals;
  elapsed_time += delta;
  double ratio = delta.count() / 1000000000.;

  if (elapsed_time >= 1s) {
    tick_count++;
    elapsed_time -= 1s;
    fuel -= power;
    velocity.y -= MARS_GRAVITY;
    velocity.x += power * std::cos(rotate * DEG_TO_RAD);
    velocity.y += power * std::sin(rotate * DEG_TO_RAD);
    position += {static_cast<int>(velocity.x), static_cast<int>(velocity.y)};
  }
}
