#pragma once

#include <fmt/format.h>
#include <fmt/color.h>

#define VREN_LOG_LEVEL_DEBUG 3
#define VREN_LOG_LEVEL_INFO  2
#define VREN_LOG_LEVEL_WARN  1
#define VREN_LOG_LEVEL_ERR   0

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_DEBUG
#	define VREN_DEBUG(m, ...) fmt::print(fmt::fg(fmt::color::gray), m, __VA_ARGS__)
#else
#	define VREN_DEBUG(m, ...)
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_INFO
#	define VREN_INFO(m, ...) fmt::print(m, __VA_ARGS__)
#else
#	define VREN_INFO(m, ...)
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_WARN
#	define VREN_WARN(m, ...) fmt::print(stderr, fmt::color::yellow, m, __VA_ARGS__)
#else
#	define VREN_WARN(m, ...)
#endif

#if VREN_LOG_LEVEL >= VREN_LOG_LEVEL_ERR
#	define VREN_ERR(m, ...) fmt::print(stderr, fmt::color::red, m, __VA_ARGS__)
#else
#	define VREN_ERR(m, ...)
#endif
