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

  world_data(view_transform to_screen);

  void set_file_data(file_data loaded);
  void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
  void update();

  inline coordinate_list ground_line() const { return loaded_.ground_line; }
  inline simulation_data initial_values() const { return loaded_.initial_values; }

  std::atomic<bool> generating{false};
  unsigned int population_size{100};
  unsigned int generation_count{200};

private:
  file_data loaded_;

  enum class status { game, genetic_algo } status_{status::genetic_algo};
};
