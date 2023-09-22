#version 460

layout(location = 0) in vec3 v_position;
layout(location = 1) in flat uint v_color;

layout(location = 0) out vec4 f_color;

void main()
{
    f_color = vec4(
    ((v_color & 0xff0000u) >> 16) / 255.0f,
    ((v_color & 0xff00u) >> 8) / 255.0f,
    (v_color & 0xffu) / 255.0f,
    1.0
    );
}
