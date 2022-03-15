#pragma once

#define VREN_NAME ("vren")
#define VREN_VERSION VK_MAKE_VERSION(1, 0, 0)

#define VREN_DESCRIPTOR_POOL_SIZE 32

#define VREN_MAX_FRAMES_IN_FLIGHT 4

#define VREN_POINT_LIGHTS_BUFFER_SIZE       (2 * 1024 * 1024)
#define VREN_DIRECTIONAL_LIGHTS_BUFFER_SIZE (1 * 1024 * 1024)
#define VREN_SPOT_LIGHTS_BUFFER_SIZE        (1 * 1024 * 1024)

// ------------------------------------------------------------------------------------------------
// Descriptor sets
// ------------------------------------------------------------------------------------------------

/* Material */
#define VREN_MATERIAL_DESCRIPTOR_SET 0

/* Light array */
#define VREN_LIGHT_ARRAY_DESCRIPTOR_SET  1

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