#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_mesh_shader : require

#define THREADS_NUM 32

#define UINT32_MAX 4294967295u

#define OCCLUSION_CULLING

#include "common.glsl"

layout(local_size_x = THREADS_NUM, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform PushConstants
{
    Camera camera;
};

layout(set = 2, binding = 3) buffer readonly MeshletBuffer
{
    Meshlet meshlets[];
};

layout(set = 2, binding = 4) buffer readonly InstancedMeshletBuffer
{
    InstancedMeshlet instanced_meshlets[];
};

layout(set = 2, binding = 5) buffer readonly InstanceBuffer
{
    MeshInstance instances[];
};

layout(set = 4, binding = 0) uniform sampler2D depth_buffer_pyramid;

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool project_sphere(vec3 C, float r, out vec4 aabb)
{
    if (C.z + r < camera.position.z + camera.z_near)
    {
        return false;
    }

    vec2 cx = -C.xz;
    vec2 vx = vec2(sqrt(dot(cx, cx) - r * r), r);
    vec2 minx = mat2(vx.x, vx.y, -vx.y, vx.x) * cx;
    vec2 maxx = mat2(vx.x, -vx.y, vx.y, vx.x) * cx;

    vec2 cy = -C.yz;
    vec2 vy = vec2(sqrt(dot(cy, cy) - r * r), r);
    vec2 miny = mat2(vy.x, vy.y, -vy.y, vy.x) * cy;
    vec2 maxy = mat2(vy.x, -vy.y, vy.y, vy.x) * cy;

    float P00 = camera.projection[0][0];
    float P11 = camera.projection[1][1];

    aabb = vec4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11);
    aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

    return true;
}

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

    barrier();

    if (gl_GlobalInvocationID.x < instanced_meshlets.length())
    {
        uint instanced_meshlet_idx = gl_GlobalInvocationID.x;

        InstancedMeshlet instanced_meshlet = instanced_meshlets[instanced_meshlet_idx];
        Meshlet meshlet = meshlets[instanced_meshlet.meshlet_idx];
        MeshInstance instance = instances[instanced_meshlet.instance_idx];
        Sphere sphere = meshlet.bounding_sphere;

        bool visible = true;

#ifdef OCCLUSION_CULLING
        vec4 aabb;
        vec3 center = (instance.transform * vec4(sphere.center, 1)).xyz;

        if (project_sphere(center, sphere.radius, aabb))
        {
            ivec2 depth_buffer_pyramid_base_size = textureSize(depth_buffer_pyramid, 0);

            float width = (aabb.z - aabb.x) * depth_buffer_pyramid_base_size.x;
            float height = (aabb.w - aabb.y) * depth_buffer_pyramid_base_size.y;

            float level = floor(log2(max(width, height)));

            float depth = textureLod(depth_buffer_pyramid, (aabb.xy + aabb.zw) * 0.5, level).x;
            float sphere_depth = (camera.projection * camera.view * vec4(center + normalize(camera.position - center) * sphere.radius, 1)).z; // TODO this mess to find just the z, couldn't it be simplified?

            visible = sphere_depth > depth;
        }
        else
        {
            visible = false;
        }
#endif

        if (visible)
        {
            uint pos = atomicAdd(s_meshlet_instances_count, 1);
            o_task.instanced_meshlet_indices[pos] = instanced_meshlet_idx;
        }
    }

    barrier();

    if (gl_LocalInvocationIndex == 0)
    {
        gl_TaskCountNV = s_meshlet_instances_count;
    }
}
