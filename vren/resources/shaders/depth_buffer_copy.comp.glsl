#version 460

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform sampler2D depth_buffer;
layout(set = 0, binding = 1) layout(r32f) uniform image2D depth_buffer_pyramid_base;

void main()
{
    float depth = texture(depth_buffer, gl_GlobalInvocationID.xy).r;
    imageStore(depth_buffer_pyramid_base, ivec2(gl_GlobalInvocationID.xy), vec4(depth));
}
