#version 460

#extension GL_GOOGLE_include_directive: require
#extension GL_NV_mesh_shader: require

#include "common.glsl"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer Meshlets
{
    Meshlet meshlets[];
};

out taskNV Task
{
    uint meshlet_idx;
} task_out;

void main()
{
    gl_TaskCountNV = 0;

    uint meshlet_idx = gl_WorkGroupID.x;
    if (meshlet_idx < meshlets.length())
    {
        task_out.meshlet_idx = meshlet_idx;
        gl_TaskCountNV = 1;
    }
}

