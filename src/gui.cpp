#include "gui.hpp"
#include "game_data.hpp"
#include "load_file.hpp"
#include <imgui.h>
#include <string_view>

void draw_file_selection(game_data &data) {
  if (ImGui::Begin("File Selection")) {
    if (fs::exists(data.resource_path)) {
      auto paths = path_list(data.resource_path);
      for (const auto &file : paths) {
        if (ImGui::Selectable(file.filename().string().c_str())) {
          data.current_file = file;
          auto loaded = load_file(file);
          data.initialize(loaded);
        }
      }
    }
  }
  ImGui::End();
}

std::string_view to_string(enum simulation::status status) {
  switch (status) {
  case simulation::status::crashed:
    return "Crashed";
  case simulation::status::landed:
    return "Landed";
  case simulation::status::paused:
    return "Paused";
  case simulation::status::running:
    return "Running";
  }
  return "Unknown";
}

void draw_gui(game_data &data) {
  // Example ImGui window
  if (ImGui::Begin("Coordinates")) {
    ImGui::Text("Status: %s", to_string(data.simu.status).data());

    if (data.current_file) {
      ImGui::Text("File: %s", data.current_file->filename().string().c_str());
      ImGui::Text("Elapsed Time: %d", data.simu.tick_count);
      ImGui::Spacing();
      if (data.simu.status == simulation::status::running) {
        if (ImGui::Button("Pause")) {
          data.simu.status = simulation::status::paused;
        }
      } else if (data.simu.status == simulation::status::paused) {
        if (ImGui::Button("Resume")) {
          data.simu.status = simulation::status::running;
        }
      } else if (data.simu.status == simulation::status::landed ||
                 data.simu.status == simulation::status::crashed) {
        if (ImGui::Button("Restart")) {
          data.simu.status = simulation::status::running;
          data.simu.set_data(data.initial);
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
        data.simu.set_data(data.initial);
      }

      ImGui::Separator();
      ImGui::Columns(2);

      ImGui::Text("Position: %d, %d", data.simu.data.position.x,
                  data.simu.data.position.y);
      ImGui::Text("Velocity: %d, %d", data.simu.data.velocity.x,
                  data.simu.data.velocity.y);
      ImGui::Text("Fuel: %d", data.simu.data.fuel);
      ImGui::Text("Rotate: %d", data.simu.data.rotate);
      ImGui::Text("Power: %d", data.simu.data.power);

      ImGui::NextColumn();

      ImGui::Text("Initial Position: %d, %d", data.initial.position.x,
                  data.initial.position.y);
      ImGui::Text("Initial Velocity: %d, %d", data.initial.velocity.x,
                  data.initial.velocity.y);
      ImGui::Text("Initial Fuel: %d", data.initial.fuel);
      ImGui::Text("Initial Rotate: %d", data.initial.rotate);
      ImGui::Text("Initial Power: %d", data.initial.power);

      ImGui::Columns(1);
      ImGui::Separator();

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
