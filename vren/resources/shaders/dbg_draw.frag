#version 460

layout(location = 0) in vec3 v_position;

layout(push_constant) uniform PushConstants
{
    vec4 color;
} push_constants;

void main()
{
    gl_FragColor = vec4(color, 1.0);
}
