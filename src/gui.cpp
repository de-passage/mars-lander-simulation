#include "gui.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "game_data.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include "world.hpp"

#include <array>
#include <imgui.h>
#include <string_view>

void draw_file_selection(world_data &world) {
  ImGui::Text("Files in %s", world.configuration.resource_path.c_str());
  ImGui::Separator();
  if (fs::exists(world.configuration.resource_path)) {
    auto paths = path_list(world.configuration.resource_path);
    for (const auto &file : paths) {
      if (ImGui::Selectable(file.filename().string().c_str())) {
        world.configuration.current_file = file;
        world.set_file_data(load_file(file));
      }
    }
  }
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

void draw_coordinates(const std::vector<coordinates> &coordinates) {

  if (ImGui::BeginTable("table-coordinates", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableNextColumn();
    ImGui::TableHeader("X");
    ImGui::TableNextColumn();
    ImGui::TableHeader("Y");
    for (auto &coord : coordinates) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%.0f", coord.x);
      ImGui::TableNextColumn();
      ImGui::Text("%.0f", coord.y);
    }
  }
  ImGui::EndTable();
}

void draw_frames(const simulation &simu) {
  ImGui::Text("Frames");

  constexpr std::array headers = {"Position", "Velocity", "Fuel",
                                  "Rotate",   "Power",    "Status"};
  if (ImGui::BeginTable("Frames", headers.size())) {

    for (const auto &header : headers) {
      ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed);
    }
    for (const auto &header : headers) {
      ImGui::TableNextColumn();
      ImGui::TableHeader(header);
    }

    for (int i = 0; i < simu.frame_count(); ++i) {
      const auto &frame = simu.history()[i];
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("(%.2f, %.2f)", frame.data.position.x, frame.data.position.y);
      ImGui::TableNextColumn();
      ImGui::Text("(%.1f, %.1f)", frame.data.velocity.x, frame.data.velocity.y);
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
}

void draw_decision_history(const simulation &simu) {
  ImGui::Text("Decisions");
  ImVec2 x = ImGui::CalcTextSize("Position");
  ImGui::Dummy(x);
  for (const auto &decision : simu.decisions()) {
    ImGui::Text("Rotate: %d, Power: %d", decision.rotate, decision.power);
  }
}

void draw_history(const simulation &simu) {
  if (ImGui::Begin("Command History", nullptr,
                   ImGuiWindowFlags_HorizontalScrollbar)) {

    ImGui::Text("Frame count: %d", simu.frame_count());
    ImGui::Text("Simulation result: %s",
                to_string(simu.simulation_status()).data());
    ImGui::Separator();
    ImGui::Columns(2);
    draw_frames(simu);

    ImGui::NextColumn();
    draw_decision_history(simu);
  }
  ImGui::End();
}

void draw_playback_control(game_data &game, simulation &simu, config &config) {
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

  ImGui::SliderInt("Playback speed", &config.playback_speed, 1,
                   MAX_PLAYBACK_SPEED);

  ImGui::Separator();

  bool changed = false;
  int selected_frame = simu.current_frame();
  int last_selected = selected_frame;
  bool disable_backward = selected_frame == 0;
  bool disable_forward = selected_frame == simu.frame_count() - 1;

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
                       simu.frame_count() - 1)) {
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
    if (selected_frame >= simu.frame_count()) {
      selected_frame = simu.frame_count() - 1;
    }
  }
  if (disable_forward) {
    ImGui::EndDisabled();
  }
  if (changed) {
    simu.set_history_point(selected_frame);
  }
}

template <size_t N>
void draw_frame_data(const char (&title)[N], const simulation_data &data) {
  ImGui::Text(title);
  ImGui::Text("Position: %.2f, %.2f", data.position.x, data.position.y);
  ImGui::Text("Velocity: %.1f, %.1f", data.velocity.x, data.velocity.y);
  ImGui::Text("Fuel: %d", data.fuel);
  ImGui::Text("Rotate: %d", data.rotate);
  ImGui::Text("Power: %d", data.power);
}

void draw_lander_data(const lander &lander) {
  ImGui::Text("Lander Logical Position: %.2f, %.2f",
              lander.current_position().x, lander.current_position().y);
  ImGui::Text("Lander Logical Rotation: %.2f", lander.current_rotation());
  ImGui::Text("Lander Screen Coordinates: %.2f, %.2f",
              lander.triangle_position().x, lander.triangle_position().y);
  ImGui::Text("Lander Screen Rotation: %.2f", lander.triangle_rotation());
}

void draw_frame(game_data &game, config &config) {
  // Example ImGui window
  if (ImGui::Begin("Data")) {
    ImGui::Checkbox("Show trajectory", &config.show_trajectory);

    if (config.current_file) {
      ImGui::Text("File: %s", config.current_file->filename().string().c_str());
      ImGui::Spacing();

      draw_playback_control(game, game.simu, config);

      ImGui::Separator();

      ImGui::Columns(2);

      draw_frame_data("Current frame", game.simu.current_data());

      ImGui::NextColumn();

      draw_frame_data("Initial frame", game.initial);
      ImGui::Separator();

      draw_coordinates(game.coordinates());
    } else {
      ImGui::Text("No file selected.");
    }
  }
  ImGui::End();
}

void draw_ga_control(world_data &world) {
  if (ImGui::Begin("Genetic Algorithm")) {
    int pop_size = world.population_size;
    if (ImGui::InputInt("Population size", &pop_size)) {
      world.population_size = std::max(0, pop_size);
    }
    ImGui::BeginDisabled(world.generating);
    if (ImGui::Button("Create Generation")) {
      world.ga.play(world.population_size);
    }
    ImGui::EndDisabled();

    if (world.ga.generated()) {
      ImGui::Separator();
      ImGui::Text("Generation %zu", world.ga.current_generation_name());
      ImGui::BeginDisabled(world.generating);
      if (ImGui::Button("Next Generation")) {
        world.ga.next_generation();
      }
      int gen_count = world.generation_count;
      if (ImGui::InputInt("Generations", &gen_count)) {
        world.generation_count = std::max(0, gen_count);
      }
      if (ImGui::Button("Play")) {
        world.generating = true;
      }
      ImGui::EndDisabled();
    }
  }
  ImGui::End();
}

void draw_generic_info(world_data &world) {
  if (ImGui::Begin("File Selection")) {
    draw_file_selection(world);

    ImGui::Separator();

    draw_coordinates(world.ground_line());

    ImGui::Separator();

    draw_frame_data("Initial values", world.initial_values());
  }
  ImGui::End();
}

void draw_gui(world_data &world, config &config) {
  // draw_frame(world.game, config);
  draw_generic_info(world);
  draw_ga_control(world);
  // draw_history(world.game.simu);
}
