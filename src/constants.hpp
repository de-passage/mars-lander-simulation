#pragma once

constexpr static inline int GAME_WIDTH = 7000;
constexpr static inline int GAME_HEIGHT = 3000;

constexpr static inline double MARS_GRAVITY = 3.711;        // ms^2
constexpr static inline double MAX_VERTICAL_SPEED = 40.0;   // m/s
constexpr static inline double MAX_HORIZONTAL_SPEED = 20.0; // m/s

constexpr static inline int MAX_ROTATION = 90;              // degrees
constexpr static inline int MAX_POWER = 4;                  // power
constexpr static inline int MAX_TURN_RATE = 15;
constexpr static inline int MAX_FUEL = 2000;
constexpr static inline int MAX_DISTANCE_SQUARED = GAME_WIDTH * GAME_WIDTH + GAME_HEIGHT * GAME_HEIGHT;

constexpr static inline double DEG_TO_RAD = 0.0174533;      // 1 degree in radians

constexpr static inline int MAX_PLAYBACK_SPEED = 20;
