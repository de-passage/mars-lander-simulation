#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>
#include "math.hpp"

using coordinates = sf::Vector2f;
using coordinate_list = std::vector<coordinates>;

static_assert(Arithmetic<decltype(coordinates::x)>);
static_assert(Coordinates<coordinates>);

struct simulation_data {
  coordinates position;
  sf::Vector2f velocity;
  int fuel;
  int rotate; //< degrees, 90 to -90
  int power;  //< 0 to 4
};

