#version 460

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_texcoord;
layout(location = 3) in flat uint i_material_index;

// gBuffer
layout(location = 0) out vec3 o_position;
layout(location = 1) out vec3 o_normal;
layout(location = 2) out vec2 o_texcoord;
layout(location = 3) out uint o_material_index;

void main()
{
	o_position = i_position;
	o_normal = i_normal;
	o_texcoord = i_texcoord;
	o_material_index = i_material_index;
}
