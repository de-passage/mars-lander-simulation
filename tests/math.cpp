#include "math.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE("Coordinates") {
  struct coordinates {
    float x;
    float y;
  };
  static_assert(Coordinates<coordinates>);
  static_assert(!Coordinates<int>);

  coordinates a1{2500.f, 2700.f};
  coordinates a2{2500.f,2696.29};
  segment s1{a1,a2};

  segment t1{
    coordinates{0,100},
    coordinates{1000,500}
  };
  segment t2{
    coordinates{1000,500},
    coordinates{1500,1500}
  };
  segment t3{
    coordinates{1500,1500},
    coordinates{3000,1000}
  };

  REQUIRE_FALSE(segments_intersect(s1,t1));
  REQUIRE_FALSE(segments_intersect(s1,t2));
  REQUIRE_FALSE(segments_intersect(s1,t3));
}
