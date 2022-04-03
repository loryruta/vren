#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

layout(push_constant) uniform PushConstants
{
    mat4 cam_view;
    mat4 cam_proj;
} push_constants;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_color;

void main()
{
    v_position = a_position;
    v_color = a_color;

    gl_Position = cam_proj * cam_view * vec4(a_position, 1);
}
