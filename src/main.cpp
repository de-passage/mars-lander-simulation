#include "game_data.hpp"
#include "gui.hpp"
#include "load_file.hpp"

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include "lander.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

struct trajectory : sf::Drawable {

  trajectory(view_transform transform) : transform_{transform} {}

  void attach(simulation &simu) {
    simu.on_data_change([this, &simu] {
      assert(simu.frame_count() > 1);
      line_.clear();
      points_.clear();
      line_.resize(simu.frame_count());

      for (auto hist : simu.history()) {
        auto position = transform_.to_screen(hist.position);
        sf::CircleShape point(2.);
        point.setFillColor(sf::Color::White);
        point.setPosition(position - sf::Vector2f{1., 1.});
        points_.push_back(point);
        line_.append(sf::Vertex{position, sf::Color::White});
      }
    });
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
    target.draw(line_);
    for (const auto &point : points_) {
      target.draw(point);
    }
  }
private:
  view_transform transform_;

  sf::VertexArray line_;
  std::vector<sf::CircleShape> points_;
};

int main(int argc, const char *argv[]) try {

  constexpr int INIT_WIDTH = 700 * 1.5;
  constexpr int INIT_HEIGHT = 300 * 1.5;

  game_data data;

  if (argc == 2 && fs::exists(argv[1])) {
    data.current_file = fs::path(argv[1]);
  }

  sf::RenderWindow window(sf::VideoMode(INIT_WIDTH, INIT_HEIGHT),
                          "SFML + ImGui Example");
  data.view_size = window.getSize();
  view_transform to_screen{data.view_size.x, data.view_size.y};

  lander lander{data, to_screen};
  lander.attach(data.simu);
  trajectory traj{to_screen};
  traj.attach(data.simu);

  if (data.current_file) {
    auto loaded = load_file(data.current_file.value());
    data.initialize(loaded);
  } else {
    auto paths = path_list(data.resource_path);
    if (!paths.empty()) {
      data.current_file = paths.front();
      auto loaded = load_file(data.current_file.value());
      data.initialize(loaded);
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
  using clock = std::chrono::steady_clock;
  auto last_time = clock::now();
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);

      if (event.type == sf::Event::Closed) {
        window.close();
      } else if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::C && event.key.control) {
          // Close the window
          window.close();
        }
      }
    }

    // Start new ImGui frame
    ImGui::SFML::Update(window, deltaClock.restart());
    draw_gui(data, lander);

    // Clear SFML window
    window.clear();

    if (data.current_file) {
      auto now = clock::now();
      if (data.simu.is_running()) {
        using namespace std::chrono_literals;
        auto delta = now - last_time;
        auto elapsed_ratio = static_cast<double>((1s).count()) /
                             static_cast<double>(delta.count());

        const auto &current_data = data.simu.current_data();
        const auto &next_data = data.simu.next_data();
        lander.update(
            lander::update_data{.current_position = current_data.position,
                                .next_position = next_data.position,
                                .current_rotation = current_data.rotate,
                                .next_rotation = next_data.rotate},
            elapsed_ratio);
      }
      last_time = now;
      window.draw(lander);
      window.draw(data.line);

      if (data.show_trajectory) {
        window.draw(traj);
      }
    }
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
