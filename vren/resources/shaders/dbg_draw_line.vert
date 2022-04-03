#version 460

layout(location = 0) in vec3 i_line[2];
layout(location = 1) in vec3 i_color;

layout(push_constant) uniform PushConstants
{
    mat4 cam_view;
    mat4 cam_proj;
} push_constants;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_color;

void main()
{
    v_position = i_line[gl_VertexID % 2];
    v_color = i_color;

    gl_Position = cam_proj * cam_view * vec4(v_position, 1);
}
