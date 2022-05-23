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
bool project_sphere(vec3 C, float r, out vec4 aabb) // C in camera space
{
    float m00 = camera.projection[0][0];
    float m11 = camera.projection[1][1];
    float m22 = camera.projection[2][2];
    float m32 = camera.projection[3][2];

    if ((C.z + r) * m22 + m32 < 0.0f) // The entire sphere is clipped by the near plane
    {
   //     return false;
    }

    vec2 cx = -C.xz;
    vec2 vx = vec2(sqrt(dot(cx, cx) - r * r), r);
    vec2 minx = mat2(vx.x, vx.y, -vx.y, vx.x) * cx;
    vec2 maxx = mat2(vx.x, -vx.y, vx.y, vx.x) * cx;

    vec2 cy = -C.yz;
    vec2 vy = vec2(sqrt(dot(cy, cy) - r * r), r);
    vec2 miny = mat2(vy.x, vy.y, -vy.y, vy.x) * cy;
    vec2 maxy = mat2(vy.x, -vy.y, vy.y, vy.x) * cy;

    aabb = vec4(minx.x / minx.y * m00, miny.x / miny.y * m11, maxx.x / maxx.y * m00, maxy.x / maxy.y * m11);
    aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

    /*
    // a^ = (1, 0, 0)
    vec2 cx = C.xz;
    float cxl = length(cx);
    float tx = sqrt(dot(cx, cx) - r * r);
    vec2 minx = mat2(tx, r, -r, tx) * (cx / cxl) * tx;
    vec2 maxx = mat2(tx, -r, r, tx) * (cx / cxl) * tx;

    // a^ = (0, 1, 0)
    vec2 cy = C.yz;
    float cyl = length(cy);
    float ty = sqrt(dot(cy, cy) - r * r);
    vec2 miny = mat2(ty, r, -r, ty) * (cy / cyl) * ty;
    vec2 maxy = mat2(ty, -r, r, ty) * (cy / cyl) * ty;

    aabb = vec4(minx.x * m00, miny.x * m11, maxx.x * m00, maxx.x * m11); // To screen-space AABB
    aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // To UV (0,0) -> (1,1) (note the Y being inverted)
*/
    return true;
}

out taskNV TaskData
{
    uint instanced_meshlet_indices[THREADS_NUM];
} o_task;

shared uint s_meshlet_instances_count;

vec3 get_scale(mat4 transform)
{
    return vec3(
        length(transform[0]),
        length(transform[1]),
        length(transform[2])
    );
}

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

        vec3 center = (camera.view * instance.transform * vec4(sphere.center, 1)).xyz;

        vec3 transform_scale = get_scale(instance.transform);
        float radius = sphere.radius * max(transform_scale.x, max(transform_scale.y, transform_scale.z));

        if (project_sphere(center, radius, aabb))
        {
            ivec2 depth_buffer_pyramid_base_size = textureSize(depth_buffer_pyramid, 0);

            float width = (aabb.z - aabb.x) * depth_buffer_pyramid_base_size.x;
            float height = (aabb.w - aabb.y) * depth_buffer_pyramid_base_size.y;

            float level = floor(log2(max(width, height)));

            // Retrieve the farthest Z coordinate in the AABB containing the projected sphere
            float depth = textureLod(depth_buffer_pyramid, (aabb.xy + aabb.zw) * 0.5, level).x;

            // Project the Z coordinate of the nearest sphere point, which is the one lying in the vector that link the camera origin to the sphere center
            float m22 = camera.projection[2][2];
            float m32 = camera.projection[3][2];
            float sphere_depth = (center - normalize(center) * radius).z * m22 + m32;

            // If the nearest sphere point is closer than the farthest point in the area, then we need to draw the sphere content
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
