#include "load_file.hpp"
#include <fstream>

file_data load_file(const fs::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path.string());
  }

  simulation_data data;
  coordinate_list line;

  file >> data.position.x >> data.position.y;
  file >> data.velocity.x >> data.velocity.y;
  file >> data.fuel;
  file >> data.rotate;
  file >> data.power;

  int x, y;
  while (file >> x >> y) {
    line.push_back(coordinates{x, y});
  }

  return {
      .data = std::move(data),
      .line = std::move(line),
  };
}
