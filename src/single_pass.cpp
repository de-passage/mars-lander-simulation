#include "genetic.hpp"
#include "load_file.hpp"
#include <filesystem>
#include <iostream>
#include "threadpool.hpp"

int main(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <file>\n";
    return 1;
  }
  namespace fs = std::filesystem;

  const auto file_path = fs::path(argv[1]);
  if (!fs::exists(file_path)) {
    std::cerr << "File not found: " << argv[1] << "\n";
    return 1;
  }

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

  auto data = load_file(argv[1]);

  ga_data ga{
      data.ground_line,
      data.initial_values,
  };
  ga.simulate_initial_generation(ga_data::generation_parameters{});

  using namespace std::chrono;
  using clock = steady_clock;

  auto start = clock::now();
  auto total = clock::duration::zero();
  double generation_time = 0;
  auto max_time = clock::duration::min();
  auto min_time = clock::duration::max();
  int min_gen_n = 0;
  int max_gen_n = 0;
  ga.simulate_initial_generation(params);
  auto vals = ga.current_generation_results();
  const auto play = [&] {
    while (1) {
      ga_data::fitness_score best_score =
          std::numeric_limits<ga_data::fitness_score>::min();

      for (int i = 0; i < vals.size(); ++i) {
        auto &val = vals[i];
        if (val.success()) {
          return i;
        }
      }
      auto now = clock::now();
      auto dur = now - start;
      start = now;
      total += dur;
      if (dur < min_time) {
        min_gen_n = ga.current_generation_name();
        min_time = dur;
      }
      if (dur > max_time) {
        max_gen_n = ga.current_generation_name();
        max_time = dur;
      }
      ga.next_generation();
      vals = ga.current_generation_results();
    }
  };
  size_t idx = play();
  individual result = ga.current_generation()[idx];

  auto sec = duration_cast<seconds>(total);
  auto milli = duration_cast<milliseconds>(total % 1s);
  auto micro = duration_cast<microseconds>(total % 1ms);
  std::cout << "Found a solution in " << ga.current_generation_name()
            << " generations \n";
  std::cout << "Total time: " << sec.count() << "s " << milli.count() << "ms "
            << micro.count() << "us\n";
  std::cout << "Mean generation time: "
            << duration_cast<microseconds>(total).count() /
                   ga.current_generation_name()
            << "us\n";
  std::cout << "Min generation time: "
            << duration_cast<microseconds>(min_time).count()
            << "us (generation: " << min_gen_n << ")\n";
  std::cout << "Max generation time: "
            << duration_cast<microseconds>(max_time).count()
            << "us (generation: " << max_gen_n << ")\n";
}
