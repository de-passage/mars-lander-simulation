include(Catch)

add_executable(unit_tests math.cpp)
target_link_libraries(unit_tests PRIVATE mars-lander-lib Catch2::Catch2WithMain)

catch_discover_tests(unit_tests)
