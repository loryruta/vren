#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_mesh_shader : require

#define THREADS_NUM 32

#define UINT32_MAX 4294967295u

#include "common.glsl"

layout(local_size_x = THREADS_NUM, local_size_y = 1, local_size_z = 1) in;

layout(set = 2, binding = 4) buffer readonly InstancedMeshletBuffer
{
    InstancedMeshlet instanced_meshlets[];
};

in

out taskNV TaskData
{
    uint instanced_meshlet_indices[THREADS_NUM];
} o_task;

shared uint s_meshlet_instances_count;

void main()
{
    if (gl_LocalInvocationIndex == 0)
    {
        s_meshlet_instances_count = 0;
    }

    memoryBarrierShared();
    barrier();

    uint instanced_meshlet_idx = gl_GlobalInvocationID.x;

    if (instanced_meshlet_idx < instanced_meshlets.length())
    {
        uint pos = atomicAdd(s_meshlet_instances_count, 1);
        o_task.instanced_meshlet_indices[pos] = instanced_meshlet_idx;
    }

    barrier();

    if (gl_LocalInvocationIndex == 0)
    {
        gl_TaskCountNV = s_meshlet_instances_count;
    }
}
