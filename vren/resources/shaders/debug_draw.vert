#version 460

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in uint a_color;

layout(push_constant) uniform PushConstants
{
    Camera camera;
};

layout(location = 0) out vec3 v_position;
layout(location = 1) out flat uint v_color;

void main()
{
    v_position = a_position;
    v_color = a_color;

    gl_Position = camera.projection * camera.view * vec4(v_position, 1);
}
