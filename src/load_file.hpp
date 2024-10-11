#pragma once

#include "game_data.hpp"

struct file_data {
  simulation_data data;
  coordinate_list line;
};
file_data load_file(const fs::path &path);

