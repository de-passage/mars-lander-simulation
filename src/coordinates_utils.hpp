#pragma once

#include "math.hpp"
#include "SFML/System/Vector2.hpp"

template<Coordinates T>
sf::Vector2<coordinates_type<T>> to_sfml(const T& coord) {
  return {coord.x, coord.y};
}
