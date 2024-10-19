#pragma once

#include "simulation_data.hpp"
#include "view_transform.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include "attachable.hpp"

class lander : public sf::Drawable {

  const float lander_size = 20.f;
  const float ellipse_radius = 5.f;
  const float ellipse_scale_y = .5f;

public:
  lander(view_transform transform);
  lander(const simulation_data &data, view_transform transform);
  virtual void draw(sf::RenderTarget &target,
                    sf::RenderStates states) const override;

  struct update_data {
    const coordinates &current_position;
    const coordinates &next_position;
    int current_rotation;
    int next_rotation;
    int power{0};
  };

  void update(const update_data &data, float ratio);
  void update(const coordinates &position, float rotation);

  inline sf::Vector2f triangle_position() const {
    return lander_triangle_.getPosition();
  }
  inline float triangle_rotation() const {
    return lander_triangle_.getRotation();
  }

  inline coordinates current_position() const { return current_position_; }
  inline float current_rotation() const { return current_rotation_; }

  void attach(Attachable auto &simu) {
    simu.on_data_change([this, &simu]() {
      this->update(
          update_data{
              .current_position = simu.current_data().position,
              .next_position = simu.next_data().position,
              .current_rotation = simu.current_data().rotate,
              .next_rotation = simu.next_data().rotate,
              .power = simu.current_data().power,
          },
          0.f);
    });
  }

private:
  coordinates current_position_;
  float current_rotation_;

  sf::ConvexShape lander_triangle_;
  sf::RectangleShape lander_bottom_;
  sf::CircleShape thrust_marker_;
  int thrust_power_ = 0;

  coordinates calculate_position_(const coordinates &start,
                                   const coordinates &end, float ratio);
  void create_shapes_(const coordinates &start, float rotation);
  view_transform transform_;
};
