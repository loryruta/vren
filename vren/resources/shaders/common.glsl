#ifndef _COMMON_H
#define _COMMON_H

struct Camera
{
	vec3 position; float _pad;
	mat4 view;
	mat4 projection;
	float z_near;
	float _pad1[3];
};

// ------------------------------------------------------------------------------------------------
// Geometry
// ------------------------------------------------------------------------------------------------

struct Sphere
{
	vec3 center;
	float radius;
};

struct Vertex
{
    vec3 position; float _pad;
    vec3 normal;   float _pad1;
    vec2 texcoord; float _pad2[2];
};

struct Meshlet
{
	uint vertex_offset;
	uint vertex_count;
	uint triangle_offset;
	uint triangle_count;

	Sphere bounding_sphere;
};

struct InstancedMeshlet
{
	uint meshlet_idx;
	uint instance_idx;
	uint material_idx;
};

struct MeshInstance
{
	mat4 transform;
};

// ------------------------------------------------------------------------------------------------
// Material
// ------------------------------------------------------------------------------------------------

struct Material
{
	uint base_color_texture_idx;
	uint metallic_roughness_texture_idx;
	float _pad[2];
};

// ------------------------------------------------------------------------------------------------
// Lighting
// ------------------------------------------------------------------------------------------------

struct PointLight
{
	vec3 position;   float _pad;
	vec3 color;      float intensity;
};

struct DirectionalLight
{
	vec3 direction; float _pad;
	vec3 color;     float _pad1;
};

struct SpotLight
{
	vec3 direction; float _pad;
	vec3 color;
	float radius;
};

struct DebugDrawBufferVertex
{
	vec3 position;
	uint color;
};

#endif // _COMMON_H
