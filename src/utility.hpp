#pragma once

#include <cassert>

#ifdef NDEBUG
#define DEBUG_ONLY(...)
#else
#define DEBUG_ONLY(...) __VA_ARGS__
#endif
