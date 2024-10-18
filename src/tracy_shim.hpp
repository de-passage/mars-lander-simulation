#pragma once
#include <cstring>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define SetThreadName(name) tracy::SetThreadName(name)
#define TracyMessageStr(message) tracy::Profiler::Message(message, std::strlen(message))
#else
#define ZoneScoped
#define ZoneScopedN(name)
#define FrameMark
#define FrameMarkNamed(name)
#define SetThreadName(name)
#define TracyMessageStr(message)
#endif
