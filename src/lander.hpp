#pragma once

#include "game_data.hpp"
#include "view_transform.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

class lander : public sf::Drawable {

  const float lander_size = 20.f;

public:
  lander(game_data &data, view_transform transform);
  virtual void draw(sf::RenderTarget &target,
                    sf::RenderStates states) const override;

  struct update_data {
    const sf::Vector2f &current_position;
    const sf::Vector2f &next_position;
    int current_rotation;
    int next_rotation;
  };

  void update(const update_data &data, float ratio);

  inline sf::Vector2f triangle_position() const {
    return lander_triangle.getPosition();
  }
  inline float triangle_rotation() const {
    return lander_triangle.getRotation();
  }

  inline sf::Vector2f current_position() const { return current_position_; }
  inline float current_rotation() const { return current_rotation_; }
  void attach(simulation &simu);

private:
  coordinates current_position_;
  float current_rotation_;

  sf::ConvexShape lander_triangle;
  sf::RectangleShape lander_bottom;

  sf::Vector2f calculate_position_(const coordinates &start,
                                   const coordinates &end, float ratio);
  void create_shapes_(const coordinates &start, float rotation);
  view_transform transform_;
};
