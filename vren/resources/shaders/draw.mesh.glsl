#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_mesh_shader : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_16bit_storage : require

#include "common.glsl"

#define UINT32_MAX 4294967295u

#define MAX_ITERATIONS 256

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(triangles, max_vertices = 64, max_primitives = 124) out;

layout(push_constant) uniform PushConstants
{
    Camera camera;
};

layout(set = 2, binding = 0) buffer readonly VertexBuffer
{
    Vertex vertices[];
};

layout(set = 2, binding = 1) buffer readonly MeshletVertexBuffer
{
    uint meshlet_vertices[];
};

layout(set = 2, binding = 2) buffer readonly MeshletTriangleBuffer
{
    uint8_t meshlet_triangles[];
};

layout(set = 2, binding = 3) buffer readonly MeshletBuffer
{
    Meshlet meshlets[];
};

layout(set = 2, binding = 4) buffer readonly InstancedMeshletBuffer
{
    InstancedMeshlet instanced_meshlets[];
};

layout(set = 2, binding = 5) buffer readonly MeshInstanceBuffer
{
    MeshInstance mesh_instances[];
};

in taskNV TaskData
{
    uint instanced_meshlet_indices[32];
} task_in;

layout(location = 0) out vec3 v_position[];
layout(location = 1) out vec3 v_normal[];
layout(location = 2) out vec2 v_texcoords[];
layout(location = 3) out flat uint v_material_idx[];
layout(location = 4) out vec3 v_colors[];

uint hash(uint a)
{
    a = (a+0x7ED55D16u) + (a<<12);
    a = (a^0xC761C23Cu) ^ (a>>19);
    a = (a+0x165667B1u) + (a<<5);
    a = (a+0xD3A2646Cu) ^ (a<<9);
    a = (a+0xFD7046C5u) + (a<<3);
    a = (a^0xB55A4F09u) ^ (a>>16);
    return a;
}

void main()
{
    uint instanced_meshlet_idx = task_in.instanced_meshlet_indices[gl_WorkGroupID.x];
    if (instanced_meshlet_idx < UINT32_MAX)
    {
        InstancedMeshlet instanced_meshlet = instanced_meshlets[instanced_meshlet_idx];
        Meshlet meshlet = meshlets[instanced_meshlet.meshlet_idx];
        MeshInstance mesh_instance = mesh_instances[instanced_meshlet.instance_idx];
        uint material_idx = instanced_meshlet.material_idx;

        uint meshlet_hash = hash(instanced_meshlet.meshlet_idx);
        vec3 meshlet_color = vec3(float(meshlet_hash & 255u), float((meshlet_hash >> 8) & 255u), float((meshlet_hash >> 16) & 255u)) / 255.0;

        for (uint i = gl_LocalInvocationIndex * MAX_ITERATIONS; i < (gl_LocalInvocationIndex + 1) * MAX_ITERATIONS; i++)
        {
            if (i < meshlet.vertex_count)
            {
                Vertex vertex = vertices[meshlet_vertices[meshlet.vertex_offset + i]];

                vec4 world_position = mesh_instance.transform * vec4(vertex.position, 1.0);
                gl_MeshVerticesNV[i].gl_Position = camera.projection * camera.view * world_position;

                v_position[i] = world_position.xyz;
                v_normal[i] = vertex.normal;
                v_texcoords[i] = vertex.texcoord;
                v_material_idx[i] = 0;//material_idx;
                v_colors[i] = meshlet_color;
            }
        }

        for (uint i = gl_LocalInvocationIndex * MAX_ITERATIONS; i < (gl_LocalInvocationIndex + 1) * MAX_ITERATIONS; i++)
        {
            if (i < meshlet.triangle_count * 3)
            {
                gl_PrimitiveIndicesNV[i] = uint(meshlet_triangles[meshlet.triangle_offset + i]);
            }
        }

        if (gl_LocalInvocationIndex == 0)
        {
            gl_PrimitiveCountNV = meshlet.triangle_count;
        }
    }
    else
    {
        gl_PrimitiveCountNV = 0;
    }
}
