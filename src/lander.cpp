#include "lander.hpp"
#include "constants.hpp"

lander::lander(game_data &data)
    : data{data.simu.data}, height{data.view_size.y}, width{data.view_size.x} {
  create_shapes();
}

void lander::draw(sf::RenderTarget &target, sf::RenderStates states) const {
  target.draw(lander_triangle, states);
  target.draw(lander_bottom, states);
}

void lander::update() {
  auto position = calculate_position();
  lander_triangle.setPosition(position);
  lander_bottom.setPosition(position.x, position.y);
  lander_triangle.setRotation(data.rotate);
  lander_bottom.setRotation(data.rotate);
}

sf::Vector2f lander::calculate_position() {
  const float window_width = static_cast<float>(height);
  const float window_height = static_cast<float>(width);

  sf::Vector2f position;
  position.x =
      (static_cast<double>(data.position.x) / static_cast<double>(GAME_WIDTH)) *
      window_width;
  position.y = (1.0f - (static_cast<float>(data.position.y) /
                        static_cast<double>(GAME_HEIGHT))) *
               window_height;
  return position;
}

void lander::create_shapes() {
  auto position = calculate_position();
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

  lander_triangle = std::move(triangle);

  // Bottom marker
  sf::RectangleShape bottom_marker(sf::Vector2f(
      lander_size, 3.f)); // A line with width `size` and height `2`
  bottom_marker.setFillColor(sf::Color::Red); // Red line indicating the bottom
  bottom_marker.setOrigin(lander_size / 2.f, 0.f);
  lander_bottom = bottom_marker;
}
