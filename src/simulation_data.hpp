#pragma once

#include <vector>
#include "math.hpp"

struct coordinates {
  float x;
  float y;

  constexpr coordinates() = default;
  constexpr coordinates(double x, double y) : x(x), y(y) {}

  friend constexpr coordinates operator+(const coordinates &lhs, const coordinates &rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
  }

  friend constexpr coordinates operator-(const coordinates &lhs, const coordinates &rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
  }
};
using coordinate_list = std::vector<coordinates>;


static_assert(Coordinates<coordinates>);

struct simulation_data {
  coordinates position;
  coordinates velocity;
  int fuel;
  int rotate; //< degrees, 90 to -90
  int power;  //< 0 to 4
};

