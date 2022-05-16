#version 460

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform sampler2D depth_buffer;
layout(set = 0, binding = 1) layout(r32f) uniform image2D depth_buffer_pyramid_base;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
    ivec2 size = textureSize(depth_buffer, 0);

    if (pos.x < size.x && pos.y < size.y)
    {
        float depth = texture(depth_buffer, pos).r;
        imageStore(depth_buffer_pyramid_base, ivec2(gl_GlobalInvocationID.xy), vec4(depth));
    }
}
