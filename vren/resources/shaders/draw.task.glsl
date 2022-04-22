#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_mesh_shader : require

#include "common.glsl"

#define UINT32_MAX 4294967295u

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(set = 2, binding = 4) buffer readonly InstancedMeshletBuffer
{
    InstancedMeshlet instanced_meshlets[];
};

out taskNV TaskData
{
    uint instanced_meshlet_indices[32];
} task_out;

void main()
{
    // If at least one index within the current workgroup points to a valid instanced meshlet, then
    // we can generate the workgroups for it. Otherwise the task size is defaulted to 0.
    if (gl_GlobalInvocationID.x < instanced_meshlets.length())
    {
        task_out.instanced_meshlet_indices[gl_LocalInvocationIndex] = gl_GlobalInvocationID.x;
        gl_TaskCountNV = 32;
    }
    else
    {
        task_out.instanced_meshlet_indices[gl_LocalInvocationIndex] = UINT32_MAX;
    }
}
