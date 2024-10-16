#pragma once

#include "config.hpp"
#include "game_data.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include "trajectory.hpp"
#include <SFML/Graphics/Drawable.hpp>
#include <atomic>

struct world_data : sf::Drawable {
  view_transform transform;
  std::optional<game_data> game;
  class lander lander;
  trajectory traj;
  config configuration;

  ga_data ga;
  ga_data::generation_parameters ga_params;

  unsigned int generation_count{200};
  std::optional<unsigned int> selected_individual{std::nullopt};

  world_data(view_transform to_screen);

  void set_file_data(file_data loaded);
  void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
  void update();

  inline coordinate_list ground_line() const { return loaded_.ground_line; }
  inline simulation_data initial_values() const {
    return loaded_.initial_values;
  }

  void start() {
    generating_ = true;
    selected_individual.reset();
  }
  void pause() { generating_ = false; }

  bool generating() const { return generating_; }

  std::atomic<bool> keep_running{true};

  void update_ga_params();

  void load_params();
  void save_params();

private:
  std::atomic<bool> generating_{false};
  file_data loaded_;

  enum class status { game, genetic_algo } status_{status::genetic_algo};

  void draw_line_(const simulation::simulation_result &result,
                  sf::RenderTarget &window, sf::RenderStates states) const;
};
