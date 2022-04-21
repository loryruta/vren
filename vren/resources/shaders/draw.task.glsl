#version 460

#extension GL_GOOGLE_include_directive : require

#extension GL_NV_mesh_shader : require

#include "common.glsl"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(set = 2, binding = 4) buffer readonly InstancedMeshletBuffer
{
    InstancedMeshlet instanced_meshlets[];
};

out taskNV Task
{
    uint instanced_meshlet_idx;
} task_out;

void main()
{
    if (gl_GlobalInvocationID.x < instanced_meshlets.length())
    {
        task_out.instanced_meshlet_idx = gl_GlobalInvocationID.x;
        gl_TaskCountNV = 1;
    }
    else
    {
        gl_TaskCountNV = 0;
    }
}
