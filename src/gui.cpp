#include "gui.hpp"
#include "game_data.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include <array>
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
  case simulation::status::stopped:
    return "Stopped";
  }
  return "Unknown";
}

void draw_history(const game_data &data) {
  if (ImGui::Begin("Command History", nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {

    ImGui::Text("Frame count: %d", data.simu.frame_count());
    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::Text("Frames");

    if (ImGui::BeginTable("Frames", 5)) {

      constexpr std::array headers = {"Position", "Velocity", "Fuel", "Rotate",
                                      "Power"};
      for (const auto &header : headers) {
        ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed);
      }
      for (const auto &header : headers) {
        ImGui::TableNextColumn();
        ImGui::TableHeader(header);
      }

      for (int i = 0; i < data.simu.frame_count(); ++i) {
        const auto &frame = data.simu.history()[i];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("(%.2f, %.2f)", frame.position.x, frame.position.y);
        ImGui::TableNextColumn();
        ImGui::Text("(%.1f, %.1f)", frame.velocity.x, frame.velocity.y);
        ImGui::TableNextColumn();
        ImGui::Text("%d", frame.fuel);
        ImGui::TableNextColumn();
        ImGui::Text("%d", frame.rotate);
        ImGui::TableNextColumn();
        ImGui::Text("%d", frame.power);
      }
    }
    ImGui::EndTable();

    ImGui::NextColumn();
    ImGui::Text("Decisions");
    for (const auto &decision : data.simu.decisions()) {
      ImGui::Text("Rotate: %d, Power: %d", decision.rotate, decision.power);
    }
  }
  ImGui::End();
}

void draw_gui(game_data &data, const lander &lander) {
  // Example ImGui window
  if (ImGui::Begin("Coordinates")) {
    ImGui::Text("Status: %s", to_string(data.simu.current_status()).data());

    if (data.current_file) {
      ImGui::Text("File: %s", data.current_file->filename().string().c_str());
      ImGui::Text("Frame count: %d", data.simu.frame_count());
      ImGui::Spacing();

      if (data.simu.current_status() == simulation::status::running) {
        if (ImGui::Button("Pause")) {
          data.simu.pause();
        }
      } else if (data.simu.current_status() == simulation::status::paused) {
        if (ImGui::Button("Resume")) {
          data.simu.run();
        }
      } else if (data.simu.current_status() == simulation::status::stopped) {
        if (ImGui::Button("Start")) {
          data.simu.run();
        }
      } else if (data.simu.current_status() == simulation::status::landed ||
                 data.simu.current_status() == simulation::status::crashed) {
        if (ImGui::Button("Restart")) {
          data.simu.set_data(data.initial);
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
        data.simu.set_data(data.initial);
      }

      ImGui::Separator();

      static int selected_frame = 0;
      bool changed = false;
      if (data.simu.is_running()) {
        selected_frame = data.simu.current_frame();
      }
      int last_selected = selected_frame;
      bool disable_backward = selected_frame == 0;
      bool disable_forward = selected_frame == data.simu.frame_count() - 1;

      ImGui::Text("History size: %d", data.simu.frame_count() - 1);
      if (disable_backward) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button("<")) {
        selected_frame--;
        changed = true;
        if (selected_frame < 0) {
          selected_frame = 0;
        }
      }
      if (disable_backward) {
        ImGui::EndDisabled();
      }
      ImGui::SameLine();
      if (ImGui::SliderInt("##History", &selected_frame, 0,
                           data.simu.frame_count() - 1)) {
        // ImGui::Text("Selected frame: %d", selected_frame);
      }
      if (ImGui::IsItemActive()) {
        changed = true;
      }
      ImGui::SameLine();
      if (disable_forward) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button(">")) {
        selected_frame++;
        changed = true;
        if (selected_frame >= data.simu.frame_count()) {
          selected_frame = data.simu.frame_count() - 1;
        }
      }
      if (disable_forward) {
        ImGui::EndDisabled();
      }
      if (changed) {
        data.simu.set_history_point(selected_frame);
      }

      ImGui::Separator();

      ImGui::Columns(2);

      ImGui::Text("Position: %.2f, %.2f", data.simu.current_data().position.x,
                  data.simu.current_data().position.y);
      ImGui::Text("Velocity: %.1f, %.1f", data.simu.current_data().velocity.x,
                  data.simu.current_data().velocity.y);
      ImGui::Text("Fuel: %d", data.simu.current_data().fuel);
      ImGui::Text("Rotate: %d", data.simu.current_data().rotate);
      ImGui::Text("Power: %d", data.simu.current_data().power);

      ImGui::NextColumn();

      ImGui::Text("Initial Position: %.0f, %.0f", data.initial.position.x,
                  data.initial.position.y);
      ImGui::Text("Initial Velocity: %.0f, %.0f", data.initial.velocity.x,
                  data.initial.velocity.y);
      ImGui::Text("Initial Fuel: %d", data.initial.fuel);
      ImGui::Text("Initial Rotate: %d", data.initial.rotate);
      ImGui::Text("Initial Power: %d", data.initial.power);

      ImGui::Columns(1);
      ImGui::Text("Lander Logical Position: %.2f, %.2f",
                  lander.current_position().x, lander.current_position().y);
      ImGui::Text("Lander Logical Rotation: %.2f", lander.current_rotation());
      ImGui::Text("Lander Screen Coordinates: %.2f, %.2f",
                  lander.triangle_position().x, lander.triangle_position().y);
      ImGui::Text("Lander Screen Rotation: %.2f", lander.triangle_rotation());
      ImGui::Separator();

      if (ImGui::BeginTable("table-coordinates", 2,
                            ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed);
        for (auto &coord : data.coordinates()) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%.0f", coord.x);
          ImGui::TableNextColumn();
          ImGui::Text("%.0f", coord.y);
        }
      }
      ImGui::EndTable();
    } else {
      ImGui::Text("No file selected.");
    }
  }
  ImGui::End();
  draw_file_selection(data);
  draw_history(data);
}
