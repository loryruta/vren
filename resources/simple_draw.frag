#version 460

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 3) in vec2 v_tex_coords;

#define VREN_MATERIAL_DIFFUSE_TEXTURE_IDX 0
#define VREN_MATERIAL_SPECULAR_TEXTURE_IDX 1

#define VREN_MATERIAL_TEXTURES_COUNT 2

layout(set = 0, binding = 1) uniform sampler2D u_material_textures[VREN_MATERIAL_TEXTURES_COUNT];

layout(location = 0) out vec4 f_color;

void main()
{
    f_color = texture(u_material_textures[VREN_MATERIAL_DIFFUSE_TEXTURE_IDX], v_tex_coords);
}
