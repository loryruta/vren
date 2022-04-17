#version 460

#extension GL_GOOGLE_include_directive : require

#extension GL_NV_mesh_shader : require

#include "common.glsl"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

out taskNV Task
{
    uint meshlet_idx;
} task_out;

void main()
{
    task_out.meshlet_idx = gl_WorkGroupID.x;
    gl_TaskCountNV = 1;
}
