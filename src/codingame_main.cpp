#include "genetic.hpp"

int main() {

  ga_data ga;
  ga_data::generation_parameters params {
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
}


