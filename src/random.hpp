#pragma once
#include "tracy_shim.hpp"

#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <random>
#include <thread>

struct random_float {
  constexpr static inline size_t BUFFER_SIZE = 10000;

  random_float() {
#ifdef FIXED_SEED
    // srand(0);
#else
    srand(time(nullptr));
#endif
    random_numbers = new double[BUFFER_SIZE];
    current_number_read = random_numbers;
    current_number_write = random_numbers;
    while (current_number_write != random_numbers + BUFFER_SIZE) {
      *current_number_write++ = (double)rand() / RAND_MAX;
    }

    random_thread = std::thread([this]() {
      ZoneScopedN("Random thread");
      while (true) {
        std::unique_lock lock{write_mutex};
        if (current_number_write == random_numbers + BUFFER_SIZE) {
          current_number_write = random_numbers;
          std::this_thread::sleep_for(std::chrono::microseconds(100));
          if (stop_) {
            return;
          }
        }
        *current_number_write++ = (double)rand() / RAND_MAX;
      }
    });
  }

  std::random_device rd;
  std::mt19937 gen{rd()};

  double operator()() {
    ZoneScopedN("Random [0,1)");
    auto val = *current_number_read;
    current_number_read++;
    if (current_number_read == random_numbers + BUFFER_SIZE) {
      current_number_read = random_numbers;
    }
    return val;
  }

  void stop() {
    stop_ = true;
    need_more_numbers_.notify_one();
    if (random_thread.joinable()) {
      random_thread.join();
    }
  }

  ~random_float() {
    stop();
    std::unique_lock lock{write_mutex};
    delete[] random_numbers;
  }

private:
  double *random_numbers;
  double *current_number_read;
  double *current_number_write;
  std::thread random_thread;

  std::mutex write_mutex;
  std::condition_variable need_more_numbers_;
  bool stop_;
};
extern random_float randf;
