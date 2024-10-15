#include "gui.hpp"
#include "game_data.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include <array>
#include <imgui.h>
#include <string_view>

void draw_file_selection(game_data &data) {
  if (ImGui::Begin("File Selection")) {
    ImGui::Text("Files in %s", data.resource_path.c_str());
    ImGui::Separator();
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

std::string_view to_string(enum game_data::status status) {
  switch (status) {
  case game_data::status::running:
    return "Running";
  case game_data::status::stopped:
    return "Stopped";
  }
  return "Unknown";
}

std::string_view to_string(enum simulation::status status) {
  switch (status) {
  case simulation::status::none:
    return "None";
  case simulation::status::land:
    return "Landed";
  case simulation::status::crash:
    return "Crashed";
  case simulation::status::lost:
    return "Lost";
  }
  return "Unknown";
}

void draw_history(const game_data &data) {
  if (ImGui::Begin("Command History", nullptr,
                   ImGuiWindowFlags_HorizontalScrollbar)) {

    ImGui::Text("Frame count: %d", data.simu.frame_count());
    ImGui::Text("Simulation result: %s",
                to_string(data.simu.simulation_status()).data());
    ImGui::Separator();
    ImGui::Columns(2);
    ImGui::Text("Frames");

    constexpr std::array headers = {"Position", "Velocity", "Fuel", "Rotate",
                                    "Power", "Status"};
    if (ImGui::BeginTable("Frames", headers.size())) {

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
        ImGui::Text("(%.2f, %.2f)", frame.data.position.x,
                    frame.data.position.y);
        ImGui::TableNextColumn();
        ImGui::Text("(%.1f, %.1f)", frame.data.velocity.x,
                    frame.data.velocity.y);
        ImGui::TableNextColumn();
        ImGui::Text("%d", frame.data.fuel);
        ImGui::TableNextColumn();
        ImGui::Text("%d", frame.data.rotate);
        ImGui::TableNextColumn();
        ImGui::Text("%d", frame.data.power);
        ImGui::TableNextColumn();
        ImGui::Text("%s", to_string(frame.status).data());
      }
    }
    ImGui::EndTable();

    ImGui::NextColumn();
    ImGui::Text("Decisions");
    ImVec2 x = ImGui::CalcTextSize("Position");
    ImGui::Dummy(x);
    for (const auto &decision : data.simu.decisions()) {
      ImGui::Text("Rotate: %d, Power: %d", decision.rotate, decision.power);
    }
  }
  ImGui::End();
}

void draw_gui(game_data &game, const lander &lander) {
  // Example ImGui window
  if (ImGui::Begin("Data")) {
    ImGui::Text("Status: %s", to_string(game.simu.simulation_status()).data());
    ImGui::Checkbox("Show trajectory", &game.show_trajectory);

    if (game.current_file) {
      ImGui::Text("File: %s", game.current_file->filename().string().c_str());
      ImGui::Text("Frame count: %d", game.simu.frame_count());
      ImGui::Spacing();

      if (game.is_running()) {
        if (ImGui::Button("Stop")) {
          game.stop();
        }
      } else {
        if (ImGui::Button("Play")) {
          game.play();
        }
      }
      ImGui::SameLine();

      ImGui::SliderInt("Playback speed", &game.playback_speed, 1, 10);

      ImGui::Separator();

      bool changed = false;
      int selected_frame = game.simu.current_frame();
      int last_selected = selected_frame;
      bool disable_backward = selected_frame == 0;
      bool disable_forward = selected_frame == game.simu.frame_count() - 1;

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
                           game.simu.frame_count() - 1)) {
        game.stop();
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
        if (selected_frame >= game.simu.frame_count()) {
          selected_frame = game.simu.frame_count() - 1;
        }
      }
      if (disable_forward) {
        ImGui::EndDisabled();
      }
      if (changed) {
        game.simu.set_history_point(selected_frame);
      }

      ImGui::Separator();

      ImGui::Columns(2);

      ImGui::Text("Position: %.2f, %.2f", game.simu.current_data().position.x,
                  game.simu.current_data().position.y);
      ImGui::Text("Velocity: %.1f, %.1f", game.simu.current_data().velocity.x,
                  game.simu.current_data().velocity.y);
      ImGui::Text("Fuel: %d", game.simu.current_data().fuel);
      ImGui::Text("Rotate: %d", game.simu.current_data().rotate);
      ImGui::Text("Power: %d", game.simu.current_data().power);

      ImGui::NextColumn();

      ImGui::Text("Initial Position: %.0f, %.0f", game.initial.position.x,
                  game.initial.position.y);
      ImGui::Text("Initial Velocity: %.0f, %.0f", game.initial.velocity.x,
                  game.initial.velocity.y);
      ImGui::Text("Initial Fuel: %d", game.initial.fuel);
      ImGui::Text("Initial Rotate: %d", game.initial.rotate);
      ImGui::Text("Initial Power: %d", game.initial.power);

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
        ImGui::TableNextColumn();
        ImGui::TableHeader("X");
        ImGui::TableNextColumn();
        ImGui::TableHeader("Y");
        for (auto &coord : game.coordinates()) {
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
  draw_file_selection(game);
  draw_history(game);
}
