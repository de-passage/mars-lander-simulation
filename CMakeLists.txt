cmake_minimum_required(VERSION 3.30)

project(mars-lander LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.6 COMPONENTS graphics window system CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(Tracy CONFIG)
include (CMakeDependentOption)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
macro(set_source_list)
  set(SOURCE_LIST ${ARGN})
  list(TRANSFORM SOURCE_LIST PREPEND ${SOURCE_DIR}/)
endmacro()

################
# SHARED FILES #
################
set_source_list(
  genetic.cpp
  play.cpp
  random.cpp
  simulation.cpp
  individual.cpp
  )
add_library(genetic-algo STATIC ${SOURCE_LIST})
target_include_directories(genetic-algo PUBLIC ${SOURCE_DIR})
target_link_libraries(genetic-algo PUBLIC pthread)

set_source_list(
  game_data.cpp
  lander.cpp
  load_file.cpp
  world.cpp
)
add_library(mars-lander-lib STATIC ${SOURCE_LIST})
target_link_libraries(mars-lander-lib PUBLIC sfml-system sfml-graphics genetic-algo)
target_include_directories(mars-lander-lib PUBLIC ${SOURCE_DIR})

#################
# VISUALIZATION #
#################
set(visualizaton-libraries sfml-window ImGui-SFML::ImGui-SFML mars-lander-lib)
set_source_list(main.cpp gui.cpp)
add_executable(mars-visualizer ${SOURCE_LIST})
target_link_libraries(mars-visualizer PRIVATE ${visualizaton-libraries})

option (TRACY_ENABLE "Enable Tracy Profiler" OFF)
cmake_dependent_option(TRACY_ON_DEMAND "Enable Tracy on demand" OFF TRACY_ENABLE OFF)

# Profiling
if (TRACY_ENABLE)
  if (NOT Tracy_FOUND)
    message(FATAL_ERROR "Tracy not found")
  endif()
add_executable(profiled-visualizer ${SOURCE_LIST})
target_link_libraries(profiled-visualizer PRIVATE ${visualizaton-libraries} Tracy::TracyClient)
set_target_properties(profiled-visualizer PROPERTIES BUILD_TYPE Release)
target_compile_definitions(profiled-visualizer PRIVATE TRACY_ENABLE)
include (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if (COMPILER_SUPPORTS_MARCH_NATIVE)
  target_compile_options(profiled-visualizer PRIVATE -march=native)
endif()
endif(TRACY_ENABLE)

set_source_list(single_pass.cpp load_file.cpp)
add_executable(single-pass ${SOURCE_LIST})
target_link_libraries(single-pass PRIVATE genetic-algo)
if (TRACY_ENABLE)
  target_link_libraries(single-pass PRIVATE Tracy::TracyClient)
  target_compile_definitions(single-pass PRIVATE TRACY_ENABLE TRACY_NO_EXIT)
endif(TRACY_ENABLE)

#############
# CODINGAME #
#############
set_source_list(codingame_main.cpp)
add_executable(codingame ${SOURCE_LIST})
target_include_directories(codingame PRIVATE src)
target_link_libraries(codingame PRIVATE genetic-algo)

set(generated_file ${CMAKE_CURRENT_BINARY_DIR}/generated.cpp)
get_target_property(app_SOURCES codingame SOURCES)
get_target_property(lib_SOURCES genetic-algo SOURCES)
list(APPEND app_SOURCES ${lib_SOURCES})
get_target_property(app_INCLUDES codingame INCLUDE_DIRECTORIES)
list(TRANSFORM app_INCLUDES PREPEND -I)

set(include_files
  simulation.hpp
  simulation_data.hpp
  genetic.hpp
  individual.hpp
  random.hpp
  tracy_shim.hpp
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
