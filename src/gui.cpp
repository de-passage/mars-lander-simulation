#include "gui.hpp"
#include "game_data.hpp"
#include "load_file.hpp"
#include <algorithm>
#include <imgui.h>

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
          auto loaded = load_file(file);
          data.update_coordinates(std::move(loaded.line));
          data.initial = loaded.data;
        }
      }
    }
  }
  ImGui::End();
}

void draw_gui(game_data &data) {
  // Example ImGui window
  if (ImGui::Begin("Coordinates")) {

    ImGui::Text("Position: %d, %d", data.initial.position.x, data.initial.position.y);
    ImGui::Text("Velocity: %d, %d", data.initial.velocity.x, data.initial.velocity.y);
    ImGui::Text("Fuel: %d", data.initial.fuel);
    ImGui::Text("Rotate: %d", data.initial.rotate);
    ImGui::Text("Power: %d", data.initial.power);

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
