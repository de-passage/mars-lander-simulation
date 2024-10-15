#include "genetic.hpp"

#include "random.hpp"

individual random_individual() {
  individual ind;
  for (auto &gene : ind.genes) {
    gene.rotate = randf();
    gene.power = randf();
  }
  return ind;
}

generation random_generation(size_t size) {
  generation gen;
  for (size_t i = 0; i < size; ++i) {
    gen.push_back(random_individual());
  }
  return gen;
}

