#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <optional>
#include <utility>

template <class T>
concept Coordinates = requires(T t) {
  requires std::is_arithmetic_v<decltype(t.x)>;
  requires std::is_same_v<decltype(t.x), decltype(t.y)>;
};

template <Coordinates T> using coordinates_type = decltype(std::decay_t<T>::x);

template <Coordinates T> struct segment {
  T start;
  T end;

  constexpr segment() = default;
  constexpr segment(T start, T end)
      : start{std::move(start)}, end{std::move(end)} {}
  constexpr segment(const segment&) = default;
  constexpr segment(segment&&) = default;
  constexpr segment& operator=(const segment&) = default;
  constexpr segment& operator=(segment&&) = default;

  constexpr bool operator==(const segment &other) const {
    return start == other.start && end == other.end;
  }
};

template <Coordinates T> segment(T, T) -> segment<T>;

template <Coordinates T, Coordinates U>
requires std::is_same_v<std::decay_t<T>, std::decay_t<U>> coordinates_type<T>
constexpr distance_squared(T &&p1, U &&p2) {
  return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}
template <Coordinates T>
constexpr coordinates_type<T> distance_squared(const segment<T> &s1) {
  return distance_squared(s1.start, s1.end);
}

template <Coordinates T> constexpr bool on_segment(const segment<std::decay_t<T>> &s, T &&q) {
  return distance_squared(s.start, q) + distance_squared(s.end, q) ==
         distance_squared(s);
}

template <Coordinates T>
constexpr coordinates_type<T> signed_area_doubled(T &&p1, T &&p2, T &&p3) {
  return ((p2.x - p1.x) * (p3.y - p1.y)) - ((p2.y - p1.y) * (p3.x - p1.x));
}

template <Coordinates T>
constexpr bool segments_intersect(const segment<T> &s1, const segment<T> &s2) {
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

template <Coordinates T> constexpr auto distance(const segment<T> &s) {
  return std::sqrt(distance_squared(s));
}

template <Coordinates T, Coordinates U>
requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
constexpr auto distance(T &&p1, U &&p2) {
  return std::sqrt(distance_squared(std::forward<T>(p1), std::forward<U>(p2)));
}

template<Coordinates T>
constexpr coordinates_type<T> dot(const T &s1, const T& s2) {
  return s1.x * s2.x + s1.y * s2.y;
}

template<Coordinates T> requires requires (T s1, T s2) {
  { s1 - s2 } -> std::same_as<T>;
}
constexpr T sub(const T &s1, const T& s2) {
  return s1 - s2;
}
template<Coordinates T> requires (!requires (T s1, T s2) {
  { s1 - s2 } -> std::same_as<T>;
})
constexpr T sub(const T &s1, const T& s2) {
  return {s1.x - s2.x, s1.y - s2.y};
}

template<Coordinates T> requires requires (T s1, T s2) {
  { s1 + s2 } -> std::same_as<T>;
}
constexpr T add(const T &s1, const T& s2) {
  return s1 + s2;
}
template<Coordinates T> requires (!requires (T s1, T s2) {
  { s1 + s2 } -> std::same_as<T>;
})
constexpr T add(const T &s1, const T& s2) {
  return {s1.x + s2.x, s1.y + s2.y};
}

template <Coordinates T, typename U>
requires std::is_same_v<coordinates_type<T>, coordinates_type<U>>
constexpr coordinates_type<T> distance_squared_to_segment(const segment<U> &s,
                                                          T &&p) {
  auto l2 = distance_squared(s);
  auto d = dot(sub(p, s.start), sub(s.end, s.start));
  auto t = d / l2;
  if (l2 == 0)
    return t;

  t = std::clamp(t, static_cast<decltype(t)>(0), static_cast<decltype(t)>(1));

  U projection;
  projection.x = s.start.x + t * (s.end.x - s.start.x);
  projection.y = s.start.y + t * (s.end.y - s.start.y);

  if (t < 0) {

  } else if (t > 1) {

  } else  {
    projection.x = s.start.x + t * (s.end.x - s.start.x);
    projection.y = s.start.y + t * (s.end.y - s.start.y);
  }
  return distance_squared(p, projection);
}

template<Coordinates T, typename U>
requires std::is_same_v<coordinates_type<T>, coordinates_type<U>>
constexpr T project(const segment<U>& s, T&& p) {
  auto l2 = distance_squared(s);

  if (l2 == 0)
    return p;

  auto t = ((p.x - s.start.x) * (s.end.x - s.start.x) +
            (p.y - s.start.y) * (s.end.y - s.start.y)) /
           l2;
  t = std::max(static_cast<decltype(t)>(0),
               std::min(static_cast<decltype(t)>(1), t));
  U projection;
  projection.x = s.start.x + t * (s.end.x - s.start.x);
  projection.y = s.start.y + t * (s.end.y - s.start.y);
  return projection;
}

template <Coordinates T, typename U>
requires std::is_same_v<coordinates_type<T>, coordinates_type<U>>
constexpr coordinates_type<T> distance_to_segment(const segment<U> &s, T &&q) {
  return std::sqrt(distance_squared_to_segment(s, std::forward<T>(q)));
}

template <Coordinates T>
constexpr std::optional<T> intersection(const segment<T> &s1,
                                        const segment<T> &s2) {
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
      static_cast<double>((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) /
      static_cast<double>(denominator);

  if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
    return T{static_cast<U>(x1 + t * (x2 - x1)),
             static_cast<U>(y1 + t * (y2 - y1))};
  }
  return std::nullopt;
}

template <Coordinates T> constexpr T midpoint(const segment<T> &s) {
  return T{(s.start.x + s.end.x) / 2, (s.start.y + s.end.y) / 2};
}

template <Coordinates T, Coordinates U>
requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
constexpr std::decay_t<T> midpoint(T &&p1, U &&p2) {
  return std::decay_t<T>{(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};
}

template <class T> constexpr T normalize(T value, T min, T max) {
  return (value - min) / (max - min);
}

template<class C, class T>
constexpr T mean(const C &c) noexcept {
  T sum = static_cast<T>(0);
  for (const auto &v : c) {
    sum += static_cast<T>(v);
  }
  return sum / static_cast<T>(c.size());
}

template<class C>
constexpr auto mean(const C &c) noexcept {
  return mean<C, typename C::value_type>(c);
}

template<class C, class T>
constexpr T variance(const C &c) noexcept {
  T m = mean(c);
  T sum = static_cast<T>(0);
  for (const auto &v : c) {
    sum += (v - m) * (v - m);
  }
  return sum / static_cast<T>(c.size());
}

template<class C, class T>
constexpr T variance(const C &c, T mean) noexcept {
  T sum = static_cast<T>(0);
  for (const auto &v : c) {
    sum += (v - mean) * (v - mean);
  }
  return sum / static_cast<T>(c.size());
}

template<class C>
constexpr auto variance(const C &c) noexcept {
  return variance<C, typename C::value_type>(c);
}

template<class C>
constexpr double standard_deviation(const C &c) noexcept {
  return std::sqrt(variance<C>(c));
}

template<class C, class T>
constexpr double standard_deviation(const C &c, T mean) noexcept {
  return std::sqrt(variance<C, T>(c, mean));
}

