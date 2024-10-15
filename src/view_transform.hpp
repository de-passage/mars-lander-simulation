#pragma once

#include "constants.hpp"
#include "math.hpp"

struct view_transform {
  unsigned int window_width;
  unsigned int window_height;

  constexpr view_transform(unsigned int window_width, unsigned int window_height)
      : window_width(window_width), window_height(window_height) {}
  constexpr view_transform(const view_transform &) = default;
  constexpr view_transform(view_transform &&) = default;
  constexpr view_transform& operator=(view_transform &&) = default;
  constexpr view_transform& operator=(const view_transform &) = default;

  template <Coordinates C> constexpr std::decay_t<C> to_screen(C &&logical) const {
    std::decay_t<C> ret;
    ret.x = (logical.x / static_cast<float>(GAME_WIDTH)) * static_cast<float>(window_width);
    ret.y =
        (1.0f - (logical.y / static_cast<float>(GAME_HEIGHT))) * static_cast<float>(window_height);
    return ret;
  }
};
