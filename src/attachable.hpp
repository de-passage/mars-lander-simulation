#pragma once

#include <utility>

template<class T>
concept Attachable = requires(T t) {
  { t.on_data_change([]{}) };
};
