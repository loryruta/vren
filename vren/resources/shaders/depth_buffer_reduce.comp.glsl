#version 460

#define INFINITY 1e35

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0) layout(r32f) uniform image2D from_image;
layout(set = 0, binding = 1) layout(r32f) uniform image2D to_image;

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    ivec2 from_size = imageSize(from_image);
    ivec2 to_size = imageSize(to_image);

    if (pos.x < to_size.x && pos.y < to_size.y)
    {
        float max_depth = 0.0f;
        for (int x = 0; x < 2; x++)
        {
            for (int y = 0; y < 2; y++)
            {
                ivec2 from_pos = pos.xy * 2 + ivec2(x, y);
                if (from_pos.x < from_size.x && from_pos.y < from_size.y)
                {
                    float depth = imageLoad(from_image, from_pos).r;
                    max_depth = max(max_depth, depth);
                }
            }
        }

        imageStore(to_image, pos, vec4(max_depth));
    }
}
