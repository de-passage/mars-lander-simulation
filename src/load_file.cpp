#include "load_file.hpp"
#include <fstream>

namespace fs = std::filesystem;

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
    line.push_back(coordinates{static_cast<float>(x), static_cast<float>(y)});
  }

  return {
      .data = std::move(data),
      .line = std::move(line),
  };
}

std::vector<fs::path> path_list(const fs::path &source) {
  auto files = fs::directory_iterator(source);
  std::vector<fs::path> paths;

  for (const auto &file : files) {
    if (file.is_regular_file()) {
      paths.push_back(file.path());
    }
  }
  std::ranges::sort(paths);
  return paths;
}
