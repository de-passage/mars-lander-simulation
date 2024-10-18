#include "genetic.hpp"
#include "simulation_data.hpp"

#include <iostream>

int main() {

  ga_data::generation_parameters params{
      .mutation_rate = .02,
      .elitism_rate = .14,
      .population_size = 100,
      .fuel_weight = .1,
      .vertical_speed_weight = 1.,
      .horizontal_speed_weight = .98,
      .distance_weight = 1.,
      .rotation_weight = .1,
      .elite_multiplier = 5.,
      .stdev_threshold = .1,
  };

  std::vector<coordinates> points;
  int surface_n; // the number of points used to draw the surface of Mars.
  std::cin >> surface_n;
  std::cin.ignore();
  points.reserve(surface_n);
  for (int i = 0; i < surface_n; i++) {
    int land_x; // X coordinate of a surface point. (0 to 6999)
    int land_y; // Y coordinate of a surface point. By linking all the points
                // together in a sequential fashion, you form the surface of
                // Mars.
    std::cin >> land_x >> land_y;
    std::cin.ignore();
    points.emplace_back(land_x, land_y);
  }

  // game loop
  int x;
  int y;
  int h_speed; // the horizontal speed (in m/s), can be negative.
  int v_speed; // the vertical speed (in m/s), can be negative.
  int fuel;    // the quantity of remaining fuel in liters.
  int rotate;  // the rotation angle in degrees (-90 to 90).
  int power;   // the thrust power (0 to 4).
    std::cin >> x >> y >> h_speed >> v_speed >> fuel >> rotate >> power;

  simulation_data initial_data{
      .position = {(float)x, (float)y},
      .velocity = {(float)h_speed, (float)v_speed},
      .fuel = fuel,
      .rotate = rotate,
      .power = power,
  };

  segment<coordinates> landing = [&] () -> segment<coordinates> {
    for (size_t i = 0; i < points.size() - 1; ++i) {
      if (points[i].y == points[i + 1].y) {
        return segment{points[i], points[i + 1]};
      }
    }
    return {{-1, -1}, {-1, -1}};
  }();

  ga_data ga(points, initial_data);
  using namespace std::chrono;
  using namespace std::chrono_literals;
  using clock = steady_clock;

  auto start = clock::now();
  auto total = clock::duration::zero();
  ga.simulate_initial_generation(params);
  auto vals = ga.current_generation_results();
  const auto play = [&] {
    while (1) {
      ga_data::fitness_score best_score = std::numeric_limits<ga_data::fitness_score>::min();
      int best_idx = 0;
      for (int i = 0; i < vals.size(); ++i) {
        auto &val = vals[i];
        if (val.success()) {
          return i;
        }
        auto score = ga_data::compute_fitness_values(val, params, landing).score;
        if (score > best_score) {
          best_score = score;
          best_idx = i;
        }
      }
      auto now = clock::now();
      auto dur = now - start;
      start = now;
      total += dur;
      auto avg = total / ga.current_generation_name();
      if (total >= (100ms - avg)) {
        std::cerr << "Premature return because too slow, processed "
          << ga.current_generation_name() << " generations\n";
        return best_idx;
      }
      if (ga.current_generation_name() % 2 == 0) {
        std::cerr << "Generation " << ga.current_generation_name()
          << " took: " << duration_cast<microseconds>(dur).count()
          << "us\nTotal: " << duration_cast<microseconds>(total).count()
          << "us\n";
        std::cerr << "Best score this generation: " << best_score << "\n";
      }
      ga.next_generation();
      vals = ga.current_generation_results();
    }
  };
  size_t idx = play();
  individual result = ga.current_generation()[idx];

  int current_frame = 0;
  while (1) {
    simulation_data current_data {
      .position = {(float)x, (float)y},
      .velocity = {(float)h_speed, (float)v_speed},
      .fuel = fuel,
      .rotate = rotate,
      .power = power,
    };
    auto d = result(current_data, points, current_frame++);
    // Rotation is inverted on codingame
    std::cout << d.rotate << " " << d.power << "\n";
    std::cin >> x >> y >> h_speed >> v_speed >> fuel >> rotate >> power;
    std::cin.ignore();
  }
}
