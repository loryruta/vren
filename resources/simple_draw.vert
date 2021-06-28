#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;

void main()
{
    v_position = a_position;
    v_normal = a_normal;

    gl_Position = vec4(v_position, 1.0);
}
