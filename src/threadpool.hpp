#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "simulation.hpp"

struct thread_pool {
  using task = std::packaged_task<simulation::result()>;

  thread_pool(size_t threads = std::thread::hardware_concurrency())
      : threads_{} {
    assert(threads > 0);
    threads_.reserve(threads);
    for (int i = 0; i < threads; ++i) {
      threads_.emplace_back([this]() {
        while (true) {
          task t;
          {
            std::unique_lock lock{mutex_};
            has_tasks_.wait(lock,
                            [this] { return !tasks_.empty() || please_stop_; });
            if (please_stop_ || tasks_.empty()) {
              return;
            }
            t = std::move(tasks_.front());
            tasks_.pop();
          }
          t();
        }
      });
    }
  }
  ~thread_pool() {
    please_stop_ = true;
    for (auto &thread : threads_) {
      if (thread.joinable()) {
        has_tasks_.notify_all();
        thread.join();
      }
    }
  }

  template <class T>
  requires std::convertible_to<std::decay_t<T>, task>
  void push(T &&t) {
    std::lock_guard lock{mutex_};
    tasks_.emplace(std::forward<T>(t));
    has_tasks_.notify_one();
  }

private:
  bool please_stop_{false};
  std::condition_variable has_tasks_;
  std::mutex mutex_;
  std::vector<std::thread> threads_;
  std::queue<task> tasks_;
};
