#pragma once

#include "game_data.hpp"

struct file_data {
  simulation_data initial_values;
  coordinate_list ground_line;
};
file_data load_file(const std::filesystem::path &path);
std::vector<std::filesystem::path> path_list(const std::filesystem::path &source);
