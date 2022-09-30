#pragma once

#include <volk.h>

#include <vren.glsl>

#define VREN_NAME ("vren")
#define VREN_VERSION VK_MAKE_VERSION(1, 0, 0)

#define VREN_MAX_FRAME_IN_FLIGHT_COUNT 3

#define VREN_COLOR_BUFFER_OUTPUT_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define VREN_DEPTH_BUFFER_OUTPUT_FORMAT VK_FORMAT_D32_SFLOAT_S8_UINT

#define VREN_MAX_SCREEN_WIDTH 1920
#define VREN_MAX_SCREEN_HEIGHT 1080

#define VREN_MAX_POINT_LIGHT_COUNT (1 << 20)
#define VREN_MAX_DIRECTIONAL_LIGHT_COUNT (1 << 4)

#define VREN_MAX_MATERIAL_COUNT (1 << 16)

#define VREN_MAX_UNIQUE_CLUSTER_KEYS (1 << 17) // ~131K

// ------------------------------------------------------------------------------------------------
// NON configurable values
// ------------------------------------------------------------------------------------------------

#define VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT 256ull
