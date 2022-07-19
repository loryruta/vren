#version 460

#extension GL_KHR_vulkan_glsl : require
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;
layout(location = 3) in flat uint i_material_idx;

// gBuffer
layout(set = 0, binding = 0, rgba32f) uniform image2D gbuffer_positions;
layout(set = 0, binding = 1, rgba16f) uniform image2D gbuffer_normals;
layout(set = 0, binding = 2, rg16f)   uniform image2D gbuffer_texcoords;
layout(set = 0, binding = 3, r16ui)   uniform image2D gbuffer_material_indices;

void main()
{
	ivec2 pos = ivec2(gl_FragCoord.xy);

	imageStore(gbuffer_positions, pos, vec4(i_position, 1.0));
	imageStore(gbuffer_normals, pos, vec4(i_normal, 0.0));
	imageStore(gbuffer_texcoords, pos, vec4(i_texcoord, 0, 0));
	imageStore(gbuffer_material_indices, pos, uvec4(i_material_idx));
}
