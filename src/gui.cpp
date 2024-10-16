#include "gui.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "game_data.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include "world.hpp"

#include <array>
#include <imgui.h>
#include <iostream>
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

void draw_frames(const std::vector<simulation::tick_data> &simu) {
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

    for (int i = 0; i < simu.size(); ++i) {
      const auto &frame = simu[i];
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

void draw_decision_history(const std::vector<decision> &decisions) {
  ImGui::Text("Decisions");
  ImVec2 x = ImGui::CalcTextSize("Position");
  ImGui::Dummy(x);
  for (const auto &decision : decisions) {
    ImGui::Text("Rotate: %d, Power: %d", decision.rotate, decision.power);
  }
}

std::string crash_reason_to_string(simulation::crash_reason reason) {
  std::array reasons = {"Uneven ground", "Rotation", "Vertical speed",
                        "Horizontal speed"};
  std::string result;
  for (int i = 0; i < 4; ++i) {
    if ((reason & (1 << i)) == (1 << i)) {
      if (!result.empty()) {
        result += ", ";
      }
      result += reasons[i];
    }
  }
  return result + " " + std::to_string(reason);
}

void draw_history(const simulation &simu) {
  if (ImGui::Begin("Command History", nullptr,
                   ImGuiWindowFlags_HorizontalScrollbar)) {

    ImGui::Text("Frame count: %d", simu.frame_count());
    ImGui::Text("Simulation result: %s",
                to_string(simu.simulation_status()).data());
    if (simu.simulation_status() == simulation::status::crash) {
      ImGui::Text("Crash reason: %s",
                  crash_reason_to_string(simu.why_crash()).data());
    } else {
      ImGui::Dummy(ImGui::CalcTextSize("Crash reason: "));
    }
    ImGui::Text("Current frame: %d", simu.current_frame());
    ImGui::Separator();
    ImGui::Columns(2);
    draw_frames(simu.history());

    ImGui::NextColumn();
    draw_decision_history(simu.decisions());
  }
  ImGui::End();
}

void draw_history(const simulation::simulation_result &simu) {
  if (ImGui::Begin("Command History", nullptr,
                   ImGuiWindowFlags_HorizontalScrollbar)) {

    ImGui::Text("Simulation result: %s", to_string(simu.final_status).data());
    if (simu.final_status == simulation::status::crash) {
      ImGui::Text("Crash reason: %s",
                  crash_reason_to_string(simu.reason).data());
    } else {
      ImGui::Dummy(ImGui::CalcTextSize("Crash reason: "));
    }
    ImGui::Separator();
    ImGui::Columns(2);
    draw_frames(simu.history);

    ImGui::NextColumn();
    draw_decision_history(simu.decisions);
    ;
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

  ImGui::BeginDisabled(disable_backward);
  if (ImGui::Button("<")) {
    selected_frame--;
    changed = true;
    if (selected_frame < 0) {
      selected_frame = 0;
    }
  }
  ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::SliderInt("##History", &selected_frame, 0,
                       simu.frame_count() - 1)) {
    game.stop();
  }
  if (ImGui::IsItemActive()) {
    changed = true;
  }
  ImGui::SameLine();
  ImGui::BeginDisabled(disable_forward);
  if (ImGui::Button(">")) {
    selected_frame++;
    changed = true;
    if (selected_frame >= simu.frame_count()) {
      selected_frame = simu.frame_count() - 1;
    }
  }
  ImGui::EndDisabled();

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

bool input_rate(const char *label, float &value) {
  if (ImGui::InputFloat(label, &value)) {
    value = std::clamp(value, 0.0f, 1.0f);
    return true;
  }
  return false;
}

int draw_generation_results(const ga_data &ga) {
  auto results = ga.current_generation_results();
  std::vector<size_t> landed;
  std::vector<std::pair<size_t, ga_data::fitness_values>> fitness_values;

  for (size_t i = 0; i < results.size(); ++i) {
    if (results[i].final_status == simulation::status::land) {
      landed.push_back(i);
    }
    fitness_values.emplace_back(i, ga.calculate_fitness(results[i]));
  }

  auto mx = *std::max_element(fitness_values.begin(), fitness_values.end(),
                              [](const auto &a, const auto &b) {
                                return a.second.score < b.second.score;
                              });

  if (landed.empty()) {
    ImGui::Text("No landings yet.");
  } else {
    std::ostringstream ss;
    ss << "Landed: ";
    bool sep = false;
    for (auto i : landed) {
      if (sep) {
        ss << ", ";
      }
      ss << i;
      sep = true;
    }
    ImGui::Text("%s", ss.str().c_str());
  }

  ImGui::Text("Best score: %.2f", mx.second.score);
  ImGui::SameLine();
  ImGui::Text("Best individual: %zu", mx.first);
  ImGui::SameLine();
  if (ImGui::Button("Select")) {
    return mx.first;
  }
  return -1;
}

void draw_generation_controls(world_data &world) {
  ImGui::Text("Generation %zu", world.ga.current_generation_name());
  ImGui::BeginDisabled(world.generating());
  if (ImGui::Button("Next Generation")) {
    world.ga.next_generation();
  }

  ImGui::EndDisabled();
  int gen_count = world.generation_count;

  ImGui::BeginDisabled(world.keep_running_after_max_generation);
  if (ImGui::InputInt("Generations", &gen_count)) {
    world.generation_count = std::max(0, gen_count);
  }
  ImGui::EndDisabled();

  if (world.generating()) {
    if (ImGui::Button("Pause")) {
      world.pause();
    }
  } else {
    if (ImGui::Button("Play")) {
      world.start();
    }
  }
}

void draw_fitness_values(const ga_data::fitness_values &values) {
  ImGui::Text("Fitness values");
  ImGui::Text("Score: %.2f", values.score);
  ImGui::Spacing();

  ImGui::Columns(2);

  ImGui::Text("Raw values");
  ImGui::Text("Fuel: %.2f", values.fuel_score);
  ImGui::Text("Vertical speed: %.2f", values.vertical_speed_score);
  ImGui::Text("Horizontal speed: %.2f", values.horizontal_speed_score);
  ImGui::Text("Distance: %.2f", values.dist_score);
  ImGui::Text("Rotation: %.2f", values.rotation_score);

  ImGui::NextColumn();

  ImGui::Text("Weighted values");
  ImGui::Text("Fuel: %.2f", values.weighted_fuel_score);
  ImGui::Text("Vertical speed: %.2f", values.weighted_vertical_speed_score);
  ImGui::Text("Horizontal speed: %.2f", values.weighted_horizontal_speed_score);
  ImGui::Text("Distance: %.2f", values.weighted_dist_score);
  ImGui::Text("Rotation: %.2f", values.weighted_rotation_score);

  ImGui::Columns();
}

bool draw_algorithm_parameters(ga_data::generation_parameters &params) {
  ImGui::Text("Parameters");
  bool update_needed = false;
  update_needed |= input_rate("Mutation rate", params.mutation_rate);
  update_needed |= input_rate("Elitism rate", params.elitism_rate);
  update_needed |= input_rate("Score distance weight", params.distance_weight);
  update_needed |= input_rate("Score fuel weight", params.fuel_weight);
  update_needed |=
      input_rate("Score vspeed weight", params.vertical_speed_weight);
  update_needed |=
      input_rate("Score hspeed weight", params.horizontal_speed_weight);
  update_needed |= input_rate("Score rotation weight", params.rotation_weight);
  return update_needed;
}

void draw_ga_control(world_data &world) {
  if (ImGui::Begin("Genetic Algorithm")) {
    bool update_needed = draw_algorithm_parameters(world.ga_params);
    int pop_size = world.ga_params.population_size;
    if (ImGui::InputInt("Population size", &pop_size)) {
      update_needed |= world.ga_params.population_size = std::max(0, pop_size);
    }
    ImGui::BeginDisabled(world.generating());
    if (ImGui::Button("Create Generation")) {
      world.ga.play(world.ga_params);
    }

    ImGui::Separator();
    ImGui::EndDisabled();

    ImGui::Separator();
    bool kras = world.keep_running_after_solution;
    bool kramg = world.keep_running_after_max_generation;
    update_needed |=
        ImGui::Checkbox("Keep running after solution found", &kras);
    update_needed |=
        ImGui::Checkbox("Keep running after max generation", &kramg);
    world.keep_running_after_solution = kras;
    world.keep_running_after_max_generation = kramg;

    if (world.has_values()) {
      int individual_max_index =
          world.ga.current_generation_results().size() - 1;
      draw_generation_controls(world);

      if (world.generated()) {
        int show = draw_generation_results(world.ga);
        if (show >= 0) {
          world.selected_individual = show;
        }
      }

      ImGui::BeginDisabled(world.generating());

      bool show_individual = world.selected_individual.has_value();
      ImGui::Checkbox("Show individual", &show_individual);

      int selection =
          world.selected_individual ? *world.selected_individual : 0;
      if (ImGui::SliderInt("Selected individual", &selection, 0,
                           individual_max_index)) {
        world.selected_individual = selection;
      } else if (show_individual) {
        if (!world.selected_individual.has_value()) {
          world.selected_individual = 0;
        }
      } else {
        world.selected_individual.reset();
      }

      if (world.selected_individual.has_value()) {
        int i = *world.selected_individual;
        if (i > individual_max_index) {
          world.selected_individual.reset();
        } else {
          auto results =
              world.ga.current_generation_results()[*world.selected_individual];

          draw_frame_data("Selected individual", results.history.back().data);
          ImGui::Separator();
          draw_fitness_values(world.ga.calculate_fitness(results));
          draw_history(results);
        }
      }

      ImGui::EndDisabled();
    }
    if (update_needed) {
      world.update_ga_params();
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
