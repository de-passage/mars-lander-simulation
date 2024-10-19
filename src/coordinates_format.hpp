#pragma once

#include "math.hpp"
#include <iostream>

template <Coordinates T>
std::ostream &operator<<(std::ostream &os, const T &coord) {
  os << "(" << coord.x << ", " << coord.y << ")";
  return os;
}

template <Coordinates T>
std::ostream &operator<<(std::ostream &os, const segment<T> &seg) {
  os << seg.start << " -> " << seg.end;
  return os;
}
