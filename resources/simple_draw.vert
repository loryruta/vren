#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 3) in vec2 a_tex_coords;

layout(location = 16) in mat4 i_transform;

layout(push_constant) uniform PushConstants
{
    vec4 camera_pos;
    mat4 camera_view;
    mat4 camera_projection;
} push_constants;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 3) out vec2 v_tex_coords;

void main()
{
    vec4 world_pos = i_transform * vec4(a_position, 1.0);

    gl_Position = push_constants.camera_projection * push_constants.camera_view * world_pos;

    v_position = world_pos.xyz;
    v_normal = a_normal;
    v_tex_coords = a_tex_coords;
}
