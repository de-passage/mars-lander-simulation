#include "lander.hpp"
#include "coordinates_utils.hpp"

lander::lander(const simulation_data &current, view_transform transform)
    : transform_{transform} {
  create_shapes_(current.position, current.rotate);
}
lander::lander(view_transform transform) : transform_{transform} {
  create_shapes_({0, 0}, 0);
}

void lander::draw(sf::RenderTarget &target, sf::RenderStates states) const {
  for (int i = 0; i < thrust_power_; i++) {
    sf::CircleShape thrust(thrust_marker_);
    float offset = (i - (thrust_power_ - 1) / 2.0f) * (ellipse_radius * 2) +
                   ellipse_radius;
    thrust.setOrigin(offset, 0);
    target.draw(thrust, states);
  }
  target.draw(lander_triangle_, states);
  target.draw(lander_bottom_, states);
}

void lander::update(const update_data &data, float ratio) {
  thrust_power_ = data.power;
  auto position =
      calculate_position_(data.current_position, data.next_position, ratio);

  auto rotation =
      static_cast<float>(data.current_rotation) +
      static_cast<float>(data.next_rotation - data.current_rotation) * ratio;

  update(position, rotation);
}

void lander::update(const coordinates &position, float rotation) {
  current_position_ = position;
  auto screen_position = to_sfml(transform_.to_screen(current_position_));
  lander_triangle_.setPosition(screen_position);
  lander_bottom_.setPosition(screen_position);
  thrust_marker_.setPosition(screen_position);

  current_rotation_ = -rotation;
  lander_triangle_.setRotation(current_rotation_);
  lander_bottom_.setRotation(current_rotation_);
  thrust_marker_.setRotation(current_rotation_);
}

coordinates lander::calculate_position_(const coordinates &start,
                                         const coordinates &end, float ratio) {
  coordinates logical{
      start.x + (end.x - start.x) * ratio,
      start.y + (end.y - start.y) * ratio,
  };

  return logical;
}

void lander::create_shapes_(const coordinates &start, float rotation) {
  auto position = to_sfml(calculate_position_(start, start, 0.f));

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

  lander_triangle_ = std::move(triangle);

  // Bottom marker
  sf::RectangleShape bottom_marker(sf::Vector2f(
      lander_size, 3.f)); // A line with width `size` and height `2`
  bottom_marker.setFillColor(sf::Color::Red); // Red line indicating the bottom
  bottom_marker.setOrigin(lander_size / 2.f, 0.f);
  bottom_marker.setPosition(position);
  bottom_marker.setRotation(rotation);

  lander_bottom_ = bottom_marker;

  thrust_marker_ = sf::CircleShape(ellipse_radius);
  thrust_marker_.setScale(ellipse_scale_y, 1.f);
  thrust_marker_.setFillColor(sf::Color(255, 165, 0));
  thrust_marker_.setOrigin((lander_size - ellipse_radius) / 2, 0.f);
  thrust_marker_.setPosition(position);
  thrust_marker_.setRotation(rotation);
}
