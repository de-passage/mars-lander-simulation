#include "config.hpp"
#include "coordinates_utils.hpp"
#include "game_data.hpp"
#include "gui.hpp"
#include "lander.hpp"
#include "load_file.hpp"
#include "trajectory.hpp"
#include "world.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <thread>

void handle_events(sf::RenderWindow &window, const sf::Event &event,
                   world_data &world) {
  if (event.type == sf::Event::Closed) {
    window.close();
  } else if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::C && event.key.control) {
      // Close the window
      window.close();
    }
  }
}

void render(sf::RenderWindow &window, world_data &world) {
  draw_gui(world, world.configuration);

  world.update();
  window.draw(world);
}

struct generation_thread {
  std::thread thread;
  bool running{false};

  void start(world_data &world) {
    if (!running) {
      running = true;
      if (thread.joinable()) {
        thread.join();
      }
      thread = std::thread(&generation_thread::background_generation, this, std::ref(world));
    }
  }

  void background_generation(world_data &world) {
    while (world.generating) {
      world.ga.next_generation();
      if (world.ga.current_generation_name() >= world.generation_count) {
        world.generating = false;
      }
    }
    running = false;
  }

  ~generation_thread() {
    if (thread.joinable()) {
      thread.join();
    }
  }
};

int main(int argc, const char *argv[]) try {

  sf::RenderWindow window(sf::VideoMode::getDesktopMode(),
                          "SFML + ImGui Example");
  auto window_size = window.getSize();
  view_transform to_screen{window_size.x, window_size.y};

  world_data world{to_screen};
  generation_thread gen_thread;

  if (argc == 2 && fs::exists(argv[1])) {
    world.configuration.current_file = fs::path(argv[1]);
  }

  if (world.configuration.current_file) {
    world.set_file_data(load_file(world.configuration.current_file.value()));
  } else {
    auto paths = path_list(world.configuration.resource_path);
    if (!paths.empty()) {
      world.configuration.current_file = paths.front();
      world.set_file_data(load_file(world.configuration.current_file.value()));
    }
  }

  // Create SFML window
  window.setFramerateLimit(60);

  // Initialize ImGui-SFML
  if (!ImGui::SFML::Init(window)) {
    std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
    return 1;
  }

  sf::Clock deltaClock;
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);

      handle_events(window, event, world);
    }

    // Start new ImGui frame
    ImGui::SFML::Update(window, deltaClock.restart());

    if (world.generating && ! gen_thread.running) {
      gen_thread.start(world);
    }

    // Clear SFML window
    window.clear();
    render(window, world);
    ImGui::SFML::Render(window);

    // Display the SFML window
    window.display();
  }

  // Cleanup ImGui-SFML
  ImGui::SFML::Shutdown();

  return 0;
} catch (std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
  return 1;
}
