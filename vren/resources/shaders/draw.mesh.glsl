#version 460

#extension GL_GOOGLE_include_directive: require
#extension GL_NV_mesh_shader: require

#include "common.glsl"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 126) out;

layout(push_constant) uniform PushConstants
{
    mat4 cam_view;
    mat4 cam_proj;
} push_constants;

layout(binding = 0) buffer Meshlets
{
    Meshlet meshlets[];
};

layout(binding = 1) buffer VertexBuffer
{
    Vertex vertices[];
};

layout(binding = 2) buffer IndexBuffer
{
    uint indices[];
};

in taskNV Task
{
    uint meshlet_idx;
} task_in;

layout(location = 0) out vec3 v_positions[];
layout(location = 1) out vec3 v_normals[];
layout(location = 2) out vec3 v_colors[];
layout(location = 3) out vec2 v_texcoords[];

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

void main()
{
    uint mi = task_in.meshlet_idx;
    Meshlet meshlet = meshlets[mi];

	uint mhash = hash(mi);
    vec3 mcolor = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;

    uint ti = gl_LocalInvocationIndex.x;
    if (ti < meshlet.triangle_count)
    {
        for (int vi = 0; vi < 3; vi++)
        {
            Vertex vtx = vertices[indices[meshlet.vertex_offset + vi]];

            uint vtx_idx = ti * 3 + vi;
            vec3 vtx_pos = (push_constants.cam_proj * push_constants.cam_view * vec4(vtx.position, 1.0)).xyz;

            gl_MeshVerticesNV[vtx_idx].gl_Position = vec4(vtx_pos, 1.0); // TODO OPTIMIZE!
            gl_PrimitiveIndicesNV[vtx_idx] = vtx_idx;

            v_positions[vtx_idx] = vtx_pos;
            v_normals[vtx_idx] = vtx.normal;
            v_colors[vtx_idx] = mcolor;
            v_texcoords[vtx_idx] = vtx.texcoord;
        }
    }

    if (ti == 0)
    {
        gl_PrimitiveCountNV = meshlet.vertex_count;
    }
}




