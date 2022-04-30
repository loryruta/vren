#version 460

#define INFINITY 1e35

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0) layout(r32f) uniform image2D in_image;
layout(set = 0, binding = 1) layout(r32f) uniform image2D out_image;

void main()
{
    ivec2 position = ivec2(gl_GlobalInvocationID.xy);

    float max_depth = -INFINITY;
    for (int x = 0; x < 2; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            float depth = imageLoad(in_image, position.xy * 2 + ivec2(x, y)).r;
            max_depth = max(max_depth, depth);
        }
    }

    imageStore(out_image, position, vec4(max_depth)); // The image is supposed to be a multiple of 32 (so no "if" condition is needed)
}
