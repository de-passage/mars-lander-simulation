#include "config.hpp"
#include "gui.hpp"
#include "load_file.hpp"
#include "random.hpp"
#include "world.hpp"
#include "tracy_shim.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <thread>

void handle_events(sf::RenderWindow &window, const sf::Event &event,
                   world_data &world) {
  const auto close = [&window, &world] {
    window.close();
    world.pause_generation();
  };
  if (event.type == sf::Event::Closed) {
    close();
  } else if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::C && event.key.control) {
      close();
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
      thread = std::thread(&generation_thread::background_generation, this,
                           std::ref(world));
    }
  }

  void background_generation(world_data &world) {
    SetThreadName("Generation Thread");
    ZoneScopedN("Genetic Algorithm Thread");
    while (world.generating()) {
      ZoneScopedN("Generation Loop");
      world.next_generation();
      if (world.current_generation_name() >= world.generation_count &&
          !world.keep_running_after_max_generation) {
        TracyMessageStr("Pausing generation");
        world.pause_generation();
      } else {
        ZoneScopedN("Result Check");
        auto current = world.current_generation_name();
        for (auto &result : world.current_generation_results()) {
          if (result.final_status == simulation::status::land &&
              !world.keep_running_after_solution) {
            world.pause_generation();
          }
        }
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

int main(int argc, const char *argv[])
#ifdef NDEBUG
    try
#endif
{
  ZoneScopedN("Main");

  sf::RenderWindow window(sf::VideoMode::getDesktopMode(),
                          "SFML + ImGui Example");
  auto window_size = window.getSize();
  view_transform to_screen{window_size.x, window_size.y};

  world_data world{to_screen};
  generation_thread gen_thread;

  if (argc == 2 && fs::exists(argv[1])) {
    world.configuration.current_file = fs::path(argv[1]);
  }
  world.load_params();

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
    FrameMarkNamed("Main Loop");
    sf::Event event;
    while (window.pollEvent(event)) {

      ImGui::SFML::ProcessEvent(event);

      handle_events(window, event, world);
    }

    // Start new ImGui frame
    ImGui::SFML::Update(window, deltaClock.restart());

    if (world.generating() && !gen_thread.running) {
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

  randf.stop();
  return 0;
}
#ifdef NDEBUG
catch (std::exception &e) {
  randf.stop();
  std::cerr << "Error: " << e.what() << std::endl;
  return 1;
}
#endif
