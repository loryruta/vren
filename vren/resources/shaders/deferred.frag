#version 460

#extension GL_GOOGLE_include_directive : require

#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;
layout(location = 3) in flat uint i_material_index;

// gBuffer
layout(location = 0) out vec4 o_normal;
layout(location = 1) out vec2 o_texcoord;
layout(location = 2) out uint o_material_index;

void main()
{
	o_normal = vec4(i_normal, 1);
	o_texcoord = i_texcoord;
	o_material_index = i_material_index;
}
