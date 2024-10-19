#pragma once

#include <cassert>

#ifdef NDEBUG
#define DEBUG_ONLY(...)
#define ASSERT(expr) ((void)0)
#else
#define DEBUG_ONLY(...) __VA_ARGS__
#define ASSERT(expr) assert(expr)
#endif
