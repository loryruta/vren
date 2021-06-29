#version 460

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 f_color;

void main()
{
    f_color = vec4(v_position, 1.0);
}
