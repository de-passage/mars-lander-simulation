#include "play.hpp"

decision decide(const simulation_data &data, const std::vector<coordinates> &ground_line) {
  decision d;
  d.rotate = 0;
  d.power = 4;
  return d;
}
