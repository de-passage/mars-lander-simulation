#include "gui.hpp"
#include "game_data.hpp"
#include "load_file.hpp"
#include <algorithm>
#include <imgui.h>
#include <string_view>

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
          data.set_initial_parameters(loaded.data);
          data.status = game_data::status::paused;
        }
      }
    }
  }
  ImGui::End();
}

std::string_view to_string(enum game_data::status status) {
  switch (status) {
  case game_data::status::crashed:
    return "Crashed";
  case game_data::status::landed:
    return "Landed";
  case game_data::status::paused:
    return "Paused";
  case game_data::status::running:
    return "Running";
  }
  return "Unknown";
}

void draw_gui(game_data &data) {
  // Example ImGui window
  if (ImGui::Begin("Coordinates")) {
    ImGui::Text("Status: %s", to_string(data.status).data());

    if (data.current_file) {
      if (data.status == game_data::status::running) {
        if (ImGui::Button("Pause")) {
          data.status = game_data::status::paused;
        }
      } else if (data.status == game_data::status::paused) {
        if (ImGui::Button("Resume")) {
          data.status = game_data::status::running;
        }
      } else if (data.status == game_data::status::landed ||
                 data.status == game_data::status::crashed) {
        if (ImGui::Button("Restart")) {
          data.status = game_data::status::running;
          data.current = data.initial;
        }
      }
      if (ImGui::Button("Reset")) {
        data.current = data.initial;
      }

      ImGui::Columns(2);
      ImGui::Text("Initial Position: %d, %d", data.initial.position.x,
                  data.initial.position.y);
      ImGui::Text("Initial Velocity: %d, %d", data.initial.velocity.x,
                  data.initial.velocity.y);
      ImGui::Text("Initial Fuel: %d", data.initial.fuel);
      ImGui::Text("Initial Rotate: %d", data.initial.rotate);
      ImGui::Text("Initial Power: %d", data.initial.power);

      ImGui::NextColumn();

      ImGui::Text("Position: %d, %d", data.current.position.x,
                  data.current.position.y);
      ImGui::Text("Velocity: %d, %d", data.current.velocity.x,
                  data.current.velocity.y);
      ImGui::Text("Fuel: %d", data.current.fuel);
      ImGui::Text("Rotate: %d", data.current.rotate);
      ImGui::Text("Power: %d", data.current.power);

      ImGui::Columns(1);

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
    } else {
      ImGui::Text("No file selected.");
    }

    ImGui::End();
  }
  draw_file_selection(data);
}
