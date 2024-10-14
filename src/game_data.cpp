#include "game_data.hpp"
#include "constants.hpp"
#include "load_file.hpp"
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
void simulation::tick(duration delta) {
  using namespace std::chrono_literals;
  elapsed_time += delta;

  if (elapsed_time >= 1s) {
    tick_count++;
    elapsed_time -= 1s;
    compute_next_tick();
  } else {
    double ratio = elapsed_time.count() / 1000000000.;

    auto distance = next_data.position - data.position;
    adjusted_position = sf::Vector2f{
        static_cast<float>(data.position.x + (distance.x * ratio)),
        static_cast<float>(data.position.y + (distance.y * ratio))};

    adjusted_rotation = data.rotate + (next_data.rotate - data.rotate) * ratio;
  }
}

void simulation::set_data(simulation_data new_data) {
  next_data = std::move(new_data);
  data = next_data;
  compute_next_tick();
  elapsed_time = duration{0};
  tick_count = 0;
  status = simulation::status::paused;
}

void simulation::compute_next_tick() {
  data = next_data;
  adjusted_position = {static_cast<float>(data.position.x),
                       static_cast<float>(data.position.y)};
  adjusted_rotation = data.rotate;
  next_data.fuel = data.fuel - data.power;
  next_data.velocity.x =
      data.velocity.x + data.power * std::cos(data.rotate * DEG_TO_RAD);
  next_data.velocity.y = data.velocity.y +
                         data.power * std::sin(data.rotate * DEG_TO_RAD) -
                         MARS_GRAVITY;
  next_data.position = data.position + decltype(data.position){
                                           static_cast<int>(data.velocity.x),
                                           static_cast<int>(data.velocity.y)};
}
