#pragma once

#include "game_data.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

class lander : public sf::Drawable {

  const float lander_size = 20.f;

public:
  lander(game_data &data);
  virtual void draw(sf::RenderTarget &target,
                    sf::RenderStates states) const override;

  void update();

  inline sf::Vector2f triangle_position() const {
    return lander_triangle.getPosition();
  }
  inline float triangle_rotation() const {
    return lander_triangle.getRotation();
  }

  inline sf::Vector2f current_position() const {
    return data.adjusted_position;
  }
  inline float current_rotation() const {
    return data.adjusted_rotation;
  }

private:
  simulation &data;
  unsigned int height;
  unsigned int width;

  sf::ConvexShape lander_triangle;
  sf::RectangleShape lander_bottom;

  sf::Vector2f calculate_position();
  void create_shapes();
};

