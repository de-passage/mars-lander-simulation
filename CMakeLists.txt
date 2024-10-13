cmake_minimum_required(VERSION 3.30)

project(mars-lander LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.6 COMPONENTS graphics window system CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_LIST main.cpp gui.cpp game_data.cpp load_file.cpp lander.cpp)
list(TRANSFORM SOURCE_LIST PREPEND ${SOURCE_DIR}/)

add_executable(mars-visualizer ${SOURCE_LIST})
target_link_libraries(mars-visualizer PRIVATE sfml-system sfml-graphics sfml-window ImGui-SFML::ImGui-SFML)
