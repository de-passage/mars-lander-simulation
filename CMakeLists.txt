cmake_minimum_required(VERSION 3.30)

project(mars-lander LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.6 COMPONENTS graphics window system CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)


set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
macro(set_source_list)
  set(SOURCE_LIST ${ARGN})
  list(TRANSFORM SOURCE_LIST PREPEND ${SOURCE_DIR}/)
endmacro()

set_source_list(
  game_data.cpp
  load_file.cpp
  lander.cpp
  simulation.cpp
  play.cpp
  genetic.cpp
  random.cpp
)
add_library(mars-lander-lib OBJECT ${SOURCE_LIST})
target_link_libraries(mars-lander-lib PUBLIC sfml-system sfml-graphics)
target_include_directories(mars-lander-lib PUBLIC ${SOURCE_DIR})


set_source_list(main.cpp gui.cpp)
add_executable(mars-visualizer ${SOURCE_LIST})
target_link_libraries(mars-visualizer PRIVATE  sfml-window ImGui-SFML::ImGui-SFML mars-lander-lib)

include(CTest)
find_package(Catch2 REQUIRED)
add_subdirectory(tests)
