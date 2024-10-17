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
  genetic.cpp
  lander.cpp
  load_file.cpp
  play.cpp
  random.cpp
  simulation.cpp
  world.cpp
  individual.cpp
)
add_library(mars-lander-lib OBJECT ${SOURCE_LIST})
target_link_libraries(mars-lander-lib PUBLIC sfml-system sfml-graphics)
target_include_directories(mars-lander-lib PUBLIC ${SOURCE_DIR})


set_source_list(main.cpp gui.cpp)
add_executable(mars-visualizer ${SOURCE_LIST})
target_link_libraries(mars-visualizer PRIVATE  sfml-window ImGui-SFML::ImGui-SFML mars-lander-lib)

set_source_list(codingame_main.cpp genetic.cpp play.cpp random.cpp simulation.cpp individual.cpp)
add_executable(codingame ${SOURCE_LIST})
target_include_directories(codingame PRIVATE src)

set(generated_file ${CMAKE_CURRENT_BINARY_DIR}/generated.cpp)
get_target_property(app_SOURCES codingame SOURCES)
get_target_property(app_INCLUDES codingame INCLUDE_DIRECTORIES)
list(TRANSFORM app_INCLUDES PREPEND -I)

set(include_files
  simulation.hpp
  simulation_data.hpp
  genetic.hpp
  individual.hpp
  random.hpp
  )
list(TRANSFORM include_files PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)
add_custom_command(
  OUTPUT "${generated_file}"
  DEPENDS "${app_SOURCES}" "${include_files}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMAND
    "${CMAKE_CURRENT_LIST_DIR}/scripts/combine.bash"
    ${app_SOURCES}
    ${app_INCLUDES}
    -o "${generated_file}"
  )

add_custom_target(
  generate
  DEPENDS "${generated_file}"
)

include(CTest)
find_package(Catch2 REQUIRED)
add_subdirectory(tests)
