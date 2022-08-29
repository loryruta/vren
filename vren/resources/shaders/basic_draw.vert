#version 460

#extension GL_GOOGLE_include_directive : require

#include <common.glsl>

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoords;

layout(location = 16) in mat4 i_transform;

layout(push_constant) uniform PushConstants
{
    mat4 camera_view;
    mat4 camera_projection;
    uint material_index;
    float _pad[3];
};

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_texcoords;
layout(location = 3) out flat uint v_material_idx;

void main()
{
    vec4 pos = i_transform * vec4(a_position, 1.0);

    gl_Position = camera_projection * camera_view * pos;

    v_position = pos.xyz;
    v_normal = a_normal;
    v_texcoords = a_texcoords;
    v_material_idx = material_index;
}
