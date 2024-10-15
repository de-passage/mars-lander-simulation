#pragma once

#include <SFML/Graphics.hpp>
#include "view_transform.hpp"
#include "attachable.hpp"

struct trajectory : sf::Drawable {

  trajectory(view_transform transform) : transform_{transform} {}

  void attach(Attachable auto &simu) {
    simu.on_data_change([this, &simu] {
      assert(simu.frame_count() > 1);
      line_.clear();
      points_.clear();

      for (auto hist : simu.history()) {
        auto position = transform_.to_screen(hist.data.position);
        const auto radius = 2.f;
        sf::CircleShape point(radius);
        point.setFillColor(sf::Color::White);
        point.setPosition(position - sf::Vector2f{radius, radius});
        points_.push_back(point);
        line_.append(sf::Vertex{position, sf::Color::White});
      }
      line_.setPrimitiveType(sf::LineStrip);
    });
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
    for (const auto &point : points_) {
      target.draw(point);
    }
    target.draw(line_);
  }

private:
  view_transform transform_;

  sf::VertexArray line_;
  std::vector<sf::CircleShape> points_;
};

