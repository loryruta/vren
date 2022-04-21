#version 460

#extension GL_GOOGLE_include_directive : require

#extension GL_NV_mesh_shader : require

#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_16bit_storage : require

#include "common.glsl"

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

in taskNV Task
{
    uint instanced_meshlet_idx;
} task_in;

layout(location = 0) out vec3 v_pos ition[];
layout(location = 1) out vec3 v_normal[];
layout(location = 2) out vec2 v_texcoords[];
layout(location = 3) out flat uint v_material_idx[];

/*
uint hash(uint a)
{
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}
*/

void main()
{
    if (gl_LocalInvocationIndex.x > 0)
    {
        return; // todo
    }

    InstancedMeshlet instanced_meshlet = instanced_meshlets[task_in.instanced_meshlet_idx];

    Meshlet meshlet = meshlets[instanced_meshlet.meshlet_idx];
    MeshInstance mesh_instance = mesh_instances[instanced_meshlet.instance_idx];
    uint material_idx = instanced_meshlet.material_idx;

    //uint meshlet_hash = hash(meshlet_idx);
    //vec3 meshlet_color = vec3(float(meshlet_hash & 255u), float((meshlet_hash >> 8) & 255u), float((meshlet_hash >> 16) & 255u)) / 255.0;

    for (int i = 0; i < meshlet.vertex_count; i++)
    {
        Vertex vertex = vertices[meshlet_vertices[meshlet.vertex_offset + i]];

        gl_MeshVerticesNV[i].gl_Position = camera.projection * camera.view * mesh_instance.transform * vec4(vertex.position, 1.0);

        v_position[i] = vertex.position;
        v_normal[i] = vertex.normal;
        v_texcoords[i] = vertex.texcoord;
        v_material_idx[i] = material_idx;
    }

    for (int i = 0; i < meshlet.triangle_count * 3; i++)
    {
        gl_PrimitiveIndicesNV[i] = uint(meshlet_triangles[meshlet.triangle_offset + i]);
    }

    gl_PrimitiveCountNV = meshlet.triangle_count;
}
