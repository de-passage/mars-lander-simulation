#pragma once

#include "game_data.hpp"
#include <SFML/Graphics.hpp>

class lander : public sf::Drawable {

  const float lander_size = 20.f;

public:
  lander(game_data &data)
      : data{data.current}, height{data.view_size.y}, width{data.view_size.x} {
    create_shapes();
  }
  virtual void draw(sf::RenderTarget &target,
                    sf::RenderStates states) const override;

  void update();

private:
  simulation_data &data;
  unsigned int height;
  unsigned int width;

  sf::ConvexShape lander_triangle;
  sf::RectangleShape lander_bottom;

  sf::Vector2f calculate_position();
  void create_shapes();
};

