#version 460

#extension GL_GOOGLE_include_directive : require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define INF 1e35
#define EPS 1e-5

#define MAX_ITER 4

#define POINT_LIGHT_SIZE 0.001f
#define POINT_LIGHT_VERTEX_COUNT 6

#include "common.glsl"

layout(set = 0, binding = 0) buffer PointLightBuffer
{
    PointLight point_lights[];
};

layout(set = 0, binding = 1) buffer DebugDrawBufferVertexBuffer
{
    DebugDrawBufferVertex vertices[];
};

void set_point_light_vertices(uint i, PointLight point_light)
{
    const uint VERTEX_COUNT = 6;

    vertices[i * POINT_LIGHT_VERTEX_COUNT + 0].position = point_light.position - vec3(POINT_LIGHT_SIZE / 2.0f, 0, 0);
    vertices[i * POINT_LIGHT_VERTEX_COUNT + 1].position = point_light.position + vec3(POINT_LIGHT_SIZE / 2.0f, 0, 0);
    vertices[i * POINT_LIGHT_VERTEX_COUNT + 2].position = point_light.position - vec3(0, POINT_LIGHT_SIZE / 2.0f, 0);
    vertices[i * POINT_LIGHT_VERTEX_COUNT + 3].position = point_light.position + vec3(0, POINT_LIGHT_SIZE / 2.0f, 0);
    vertices[i * POINT_LIGHT_VERTEX_COUNT + 4].position = point_light.position - vec3(0, 0, POINT_LIGHT_SIZE / 2.0f);
    vertices[i * POINT_LIGHT_VERTEX_COUNT + 5].position = point_light.position + vec3(0, 0, POINT_LIGHT_SIZE / 2.0f);

    for (uint j = 0; j < POINT_LIGHT_VERTEX_COUNT; j++)
    {
        vertices[i * POINT_LIGHT_VERTEX_COUNT + j].color = 0xffff00;
    }
}

void main()
{
    for (uint i = gl_GlobalInvocationID.x * MAX_ITER; i < (gl_GlobalInvocationID.x + 1) * MAX_ITER; i++)
    {
        if (i < point_lights.length())
        {
            set_point_light_vertices(i, point_lights[i]);
        }
    }
}
