#pragma once

#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

struct config {
  bool show_trajectory{true};

  fs::path resource_path{"data"};
  std::optional<fs::path> current_file;
  int playback_speed = 5;
};
