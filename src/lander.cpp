#include "lander.hpp"
#include "constants.hpp"
#include <iostream>

lander::lander(game_data &data)
    : height{data.view_size.y}, width{data.view_size.x} {
  assert(height > 0);
  assert(width > 0);
  const simulation_data *current;
  if (data.simu.frame_count() > 0) {
    current = &data.simu.current_data();
  } else {
    static simulation_data default_value = {
        .position = {GAME_WIDTH / 2., 0},
        .velocity = {0, 0},
        .fuel = 100,
        .rotate = 0,
        .power = 0,
    };
    current = &default_value;
  }
  create_shapes_(current->position, current->rotate);
}

void lander::draw(sf::RenderTarget &target, sf::RenderStates states) const {
  target.draw(lander_triangle, states);
  target.draw(lander_bottom, states);
}

void lander::update(const update_data &data, float ratio) {
  current_position_ =
      calculate_position_(data.current_position, data.next_position, ratio);
  lander_triangle.setPosition(current_position_);
  lander_bottom.setPosition(current_position_);

  current_rotation_ =
      static_cast<float>(data.current_rotation) +
      static_cast<float>(data.next_rotation - data.current_rotation) * ratio;
  lander_triangle.setRotation(current_rotation_);
  lander_bottom.setRotation(current_rotation_);
}

sf::Vector2f lander::calculate_position_(const coordinates &start,
                                         const coordinates &end, float ratio) {
  const float window_width = static_cast<float>(width);
  const float window_height = static_cast<float>(height);

  const float logical_x = start.x + (end.x - start.x) * ratio;
  const float logical_y = start.y + (end.y - start.y) * ratio;

  sf::Vector2f position;
  position.x = (logical_x / static_cast<float>(GAME_WIDTH)) * window_width;
  position.y =
      (1.0f - (logical_y / static_cast<float>(GAME_HEIGHT))) * window_height;
  return position;
}

void lander::create_shapes_(const coordinates &start, float rotation) {
  auto position = calculate_position_(start, start, 0.f);
  sf::ConvexShape triangle(3); // A triangle has 3 points
  // Define the points for the triangle (relative to the position)
  triangle.setPoint(0, sf::Vector2f(-lander_size / 2.f, 0.f)); // Left point
  triangle.setPoint(2, sf::Vector2f(lander_size / 2.f, 0.f));  // Right point
  triangle.setPoint(1, sf::Vector2f(0.f, -lander_size));       // Top point
  triangle.setFillColor(sf::Color::Green); // Fill color of the triangle
  triangle.setOrigin(0.f, 0.f);

  // Optionally, set an outline for the triangle to distinguish the bottom
  triangle.setOutlineThickness(2.f);
  triangle.setOutlineColor(
      sf::Color::Yellow); // Highlight outline (e.g., bottom side)
  triangle.setPosition(position);
  triangle.setRotation(rotation);

  lander_triangle = std::move(triangle);

  // Bottom marker
  sf::RectangleShape bottom_marker(sf::Vector2f(
      lander_size, 3.f)); // A line with width `size` and height `2`
  bottom_marker.setFillColor(sf::Color::Red); // Red line indicating the bottom
  bottom_marker.setOrigin(lander_size / 2.f, 0.f);
  bottom_marker.setPosition(position);
  bottom_marker.setRotation(rotation);

  lander_bottom = bottom_marker;
}

void lander::attach(simulation &simu) {
  simu.on_data_change([this, &simu]() {
    this->update(
        {
            .current_position = simu.current_data().position,
            .next_position = simu.next_data().position,
            .current_rotation = simu.current_data().rotate,
            .next_rotation = simu.next_data().rotate,
        },
        0.f);
  });
}
