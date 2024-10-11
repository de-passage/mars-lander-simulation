#pragma once

#include "game_data.hpp"

coordinate_list load_line(const fs::path &path);
void draw_gui(game_data& data);
