#pragma once

#include "fmt/format.h"
#include "fmt/color.h"

#define VREN_LOG_LEVEL_DEBUG 3
#define VREN_LOG_LEVEL_INFO  2
#define VREN_LOG_LEVEL_WARN  1
#define VREN_LOG_LEVEL_ERROR 0

#ifndef VREN_LOG_LEVEL
#	define VREN_LOG_LEVEL VREN_LOG_LEVEL_WARN
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_DEBUG
#	define VREN_DEBUG(m, ...) fmt::print(m, __VA_ARGS__)
#else
#	define VREN_DEBUG(m, ...)
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_INFO
#	define VREN_INFO(m, ...) fmt::print(m, __VA_ARGS__)
#else
#	define VREN_INFO(m, ...)
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_WARN
#	define VREN_WARN(m, ...) fmt::print(stderr, fmt::fg(fmt::color::yellow), m, __VA_ARGS__)
#else
#	define VREN_WARN(m, ...)
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_ERROR
#	define VREN_ERROR(m, ...) fmt::print(stderr, m, __VA_ARGS__)
#else
#	define VREN_ERROR(m, ...)
#endif
