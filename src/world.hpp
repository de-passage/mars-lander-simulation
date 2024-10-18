#pragma once

#include "config.hpp"
#include "game_data.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include "trajectory.hpp"
#include <SFML/Graphics/Drawable.hpp>
#include <atomic>

struct world_data : sf::Drawable {
  world_data(view_transform to_screen);

  bool playback_in_progress() const { return game && game->is_running(); }
  void start_playback() {
    assert(game);
    game->play();
  }
  void stop_playback() { game->stop(); }
  void reset_playback() { game->reset_simulation(); }

  void set_file_data(file_data loaded);
  void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
  void update();

  inline coordinate_list ground_line() const { return loaded_.ground_line; }
  inline simulation_data initial_values() const {
    return loaded_.initial_values;
  }

  void start_generation() {
    generating_ = true;
    reset_individual_selection_();
  }
  void pause_generation() { generating_ = false; }

  bool generating() const { return generating_; }
  bool generated() const { return has_values() && !generating_; }
  bool has_values() const { return ga.generated(); }

  void update_ga_params();

  void load_params();
  void save_params();
  size_t current_generation_name() const { return ga.current_generation_name(); }

  ga_data::generation_result current_generation_results() const {
    return ga.current_generation_results();
  }

  void next_generation() {
    ga.next_generation();
  }

  void new_generation() {
    assert(!generating_);
    ga.simulate_initial_generation(ga_params);
  }

  // Playback
  simulation::result setup_currently_selected_for_playback() {
    assert(selected_individual.has_value());
    assert(loaded_.ground_line.size() > 0);
    assert(!generating_);

    auto currently_selected =
        ga.current_generation_results()[*selected_individual];

    if (!game || last_selected_ != selected_individual) {
      last_selected_ = selected_individual;
      game.emplace(transform, currently_selected, loaded_.ground_line);
      lander.attach(*game);
    }

    return currently_selected;
  }

  segment<coordinates> landing_site() const {
    return landing_site_;
  }

  // Members
  // > Generic
  view_transform transform;
  config configuration;
  // > Playback
  std::optional<game_data> game;
  class lander lander;

  // > Genetic Algorithm
  ga_data::generation_parameters ga_params;

  unsigned int generation_count{200};

  // > Generation info display
  std::optional<unsigned int> selected_individual{std::nullopt};
  std::atomic<bool> keep_running_after_solution{false};
  std::atomic<bool> keep_running_after_max_generation{true};

private:
  std::atomic<bool> generating_{false};
  file_data loaded_;
  ga_data ga;
  segment<coordinates> landing_site_;
  std::optional<unsigned int> last_selected_{std::nullopt};

  void draw_line_(const simulation::result &result,
                  sf::RenderTarget &window, sf::RenderStates states) const;

  void reset_individual_selection_() {
    selected_individual.reset();
    lander.update(loaded_.initial_values.position,
                  loaded_.initial_values.rotate);
    game.reset();
  }
};
