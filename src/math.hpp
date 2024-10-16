#pragma once

#include <cmath>
#include <concepts>
#include <optional>
#include <utility>

template <class T>
concept Coordinates = requires(T t) {
  requires std::is_arithmetic_v<decltype(t.x)>;
  requires std::is_same_v<decltype(t.x), decltype(t.y)>;
};

struct Nope {
  int x;
  double y;
};

template <Coordinates T> using coordinates_type = decltype(std::decay_t<T>::x);

template <Coordinates T> struct segment {
  T start;
  T end;

  constexpr segment() = default;
  constexpr segment(T start, T end)
      : start{std::move(start)}, end{std::move(end)} {}
};

template <Coordinates T> segment(T, T) -> segment<T>;

template <Coordinates T, Coordinates U>
requires std::is_same_v<std::decay_t<T>, std::decay_t<U>> coordinates_type<T>
distance_squared(T &&p1, U &&p2) {
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
  return ((p2.x - p1.x) * (p3.y - p1.y)) - ((p2.y - p1.y) * (p3.x - p1.x));
}

template <Coordinates T>
bool segments_intersect(const segment<T> &s1, const segment<T> &s2) {
  const auto sign = [](auto a, auto b, auto c) {
    auto s = signed_area_doubled(a, b, c);
    return s == 0 ? 0 : s > 0 ? 1 : -1;
  };
  int a1 = sign(s1.start, s1.end, s2.start);
  int a2 = sign(s1.start, s1.end, s2.end);
  int a3 = sign(s2.start, s2.end, s1.start);
  int a4 = sign(s2.start, s2.end, s1.end);

  return a1 * a2 < 0 && a3 * a4 < 0;
}

template <Coordinates T> auto distance(const segment<T> &s) {
  return std::sqrt(distance_squared(s));
}

template <Coordinates T, Coordinates U>
requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
auto distance(T &&p1, U &&p2) {
  return std::sqrt(distance_squared(std::forward<T>(p1), std::forward<U>(p2)));
}

template <Coordinates T, typename U>
requires std::is_same_v<coordinates_type<T>, coordinates_type<U>>
    coordinates_type<T> distance_squared_to_segment(const segment<U> &s,
                                                    T &&p) {
  auto l2 = distance(s);

  if (l2 == 0)
    return distance(s.start, p);

  auto t = ((p.x - s.start.x) * (s.end.x - s.start.x) +
            (p.y - s.start.y) * (s.end.y - s.start.y)) /
           l2;
  t = std::max(static_cast<decltype(t)>(0),
               std::min(static_cast<decltype(t)>(1), t));
  U projection;
  projection.x = s.start.x + t * (s.end.x - s.start.x);
  projection.y = s.start.y + t * (s.end.y - s.start.y);
  return distance_squared(p, projection);
}

template <Coordinates T, typename U>
requires std::is_same_v<coordinates_type<T>, coordinates_type<U>>
    coordinates_type<T> distance_to_segment(const segment<U> &s, T &&q) {
  return std::sqrt(distance_squared_to_segment(s, std::forward<T>(q)));
}

template <Coordinates T>
std::optional<T> intersection(const segment<T> &s1, const segment<T> &s2) {
  using U = coordinates_type<T>;
  U x1 = s1.start.x, y1 = s1.start.y;
  U x2 = s1.end.x, y2 = s1.end.y;

  U x3 = s2.start.x, y3 = s2.start.y;
  U x4 = s2.end.x, y4 = s2.end.y;

  auto denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

  if (denominator == 0) {
    return std::nullopt;
  }

  double t =
      static_cast<double>((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) /
      static_cast<double>(denominator);

  double u =
      static_cast<double>((x1 - x2) * (y1 - y2) - (y1 - y3) * (x1 - x3)) /
      static_cast<double>(denominator);

  if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
    return T{static_cast<U>(x1 + t * (x2 - x1)), static_cast<U>(y1 + t * (y2 - y1))};
  }
  return std::nullopt;
}
