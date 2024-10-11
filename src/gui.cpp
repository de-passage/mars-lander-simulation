#include "gui.hpp"
#include "game_data.hpp"
#include <algorithm>
#include <fstream>
#include <imgui.h>

coordinate_list load_line(const fs::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path.string());
  }

  coordinate_list line;

  int x, y;
  while (file >> x >> y) {
    line.push_back(coordinates{x, y});
  }

  return line;
}

void draw_file_selection(game_data &data) {
  if (ImGui::Begin("File Selection")) {
    if (fs::exists(data.resource_path)) {
      auto files = fs::directory_iterator(data.resource_path);
      std::vector<fs::path> paths;

      for (const auto &file : files) {
        if (file.is_regular_file()) {
          paths.push_back(file.path());
        }
      }
      std::ranges::sort(paths);
      for (const auto &file : paths) {
        if (ImGui::Selectable(file.filename().string().c_str())) {
          data.current_file = file;
          data.updated_coordinates(load_line(file));
        }
      }
    }
  }
  ImGui::End();
}

void draw_gui(game_data &data) {
  // Example ImGui window
  if (ImGui::Begin("Coordinates")) {

    if (ImGui::BeginTable("table-coordinates", 2,
                          ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed);
      for (size_t i = 0; i < data.coordinates.size(); ++i) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", data.coordinates[i].x);
        ImGui::TableNextColumn();
        ImGui::Text("%d", data.coordinates[i].y);
      }
    }
    ImGui::EndTable();
  }
  ImGui::End();

  draw_file_selection(data);
}
