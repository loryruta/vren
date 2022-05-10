#pragma once

#include <volk.h>

#define VREN_NAME ("vren")
#define VREN_VERSION VK_MAKE_VERSION(1, 0, 0)

#define VREN_MAX_FRAME_IN_FLIGHT_COUNT 3

#define VREN_COLOR_BUFFER_OUTPUT_FORMAT VK_FORMAT_B8G8R8A8_UNORM
#define VREN_DEPTH_BUFFER_OUTPUT_FORMAT VK_FORMAT_D32_SFLOAT_S8_UINT

#define VREN_MAX_POINT_LIGHTS       1024
#define VREN_MAX_DIRECTIONAL_LIGHTS 1024
#define VREN_MAX_SPOT_LIGHTS        1024

#define VREN_POINT_LIGHTS_BUFFER_SIZE       (2 * 1024 * 1024) // TODO derive from MAX_*_LIGHTS
#define VREN_DIRECTIONAL_LIGHTS_BUFFER_SIZE (1 * 1024 * 1024)
#define VREN_SPOT_LIGHTS_BUFFER_SIZE        (1 * 1024 * 1024)

// ------------------------------------------------------------------------------------------------
// Bindings
// ------------------------------------------------------------------------------------------------

/* Material */
#define VREN_MATERIAL_BASE_COLOR_TEXTURE_BINDING 0
#define VREN_MATERIAL_METALLIC_ROUGHNESS_TEXTURE_BINDING  1

/* Light array */
#define VREN_LIGHT_ARRAY_POINT_LIGHT_BUFFER_BINDING 0
#define VREN_LIGHT_ARRAY_DIRECTIONAL_LIGHT_BUFFER_BINDING 1
#define VREN_LIGHT_ARRAY_SPOT_LIGHT_BUFFER_BINDING 2
