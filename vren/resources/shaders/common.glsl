#ifndef _COMMON_H
#define _COMMON_H

struct Camera
{
	vec3 position; float _pad;
	mat4 view;
	mat4 projection;
};

struct Vertex
{
    vec3 position;     float _pad;
    vec3 normal;       float _pad1;
    vec2 texcoord;     float _pad2[2];
	uint material_idx; float _pad3[3];
};

struct Material
{
	uint base_color_texture_idx;         float _pad[3];
	uint metallic_roughness_texture_idx; float _pad1[3];
	vec3 base_color_factor; float _pad2;
	float metallic_factor;  float _pad3[3];
	float roughness_factor; float _pad4[3];
};

struct Meshlet
{
	uint vertex_offset;
	uint triangle_offset;
	uint vertex_count;
	uint triangle_count;
};

struct PointLight
{
	vec3 position; float _pad;
	vec3 color;    float _pad1;
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

#endif // _COMMON_H
