#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 3) in vec2 a_tex_coords;

layout(push_constant) uniform push_constants
{
    mat4 cam_view;
    mat4 cam_proj;
} PushConstants;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 3) out vec2 v_tex_coords;

void main()
{
    gl_Position = PushConstants.cam_proj * PushConstants.cam_view * vec4(a_position, 1.0);

    v_position = a_position;
    v_normal = a_normal;
    v_tex_coords = a_tex_coords;
}
