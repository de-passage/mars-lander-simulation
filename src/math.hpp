#pragma once

#include <concepts>
#include <utility>

template<class T>
concept Arithmetic = std::is_arithmetic_v<T>;
static_assert(Arithmetic<int>);

template<class T>
concept Coordinates = requires(T t) {
  std::is_same_v<decltype(t.x), decltype(t.y)>;
};

template<Coordinates T>
using coordinates_type = decltype(std::decay_t<T>::x);

template<Coordinates T>
struct segment {
  T start;
  T end;

  constexpr segment(T start, T end) : start{std::move(start)}, end{std::move(end)} {}
};

template<Coordinates T>
segment(T, T) -> segment<T>;

template <Coordinates T>
coordinates_type<T> distance_squared(Coordinates auto &&p1,
                                     Coordinates auto &&p2) {
  return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}
template <Coordinates T>
coordinates_type<T> distance_squared(const segment<T> &s1) {
  return distance_squared(s1.start, s1.end);
}

template <Coordinates T> bool on_segment(const segment<T> &s, T &&q) {
  return distance_squared(s.start, q) + distance_squared(s.end, q) ==
         distance_squared(s);
}

template <Coordinates T>
coordinates_type<T> signed_area_doubled(T &&p1, T &&p2, T &&p3) {
  return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
}

template <Coordinates T>
bool segments_intersect(const segment<T> &s1, const segment<T> &s2) {
  int a1 = signed_area_doubled(s1.start, s1.end, s2.start);
  int a2 = signed_area_doubled(s1.start, s1.end, s2.end);
  int a3 = signed_area_doubled(s2.start, s2.end, s1.start);
  int a4 = signed_area_doubled(s2.start, s2.end, s1.end);

  return a1 * a2 < 0 && a3 * a4 < 0;
}
