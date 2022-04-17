#version 460

#extension GL_GOOGLE_include_directive : require

#extension GL_EXT_nonuniform_qualifier : require

#include "common.glsl"

#define PI 3.14
#define EPSILON 0.0001
#define INFINITE 1e35

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoords;
layout(location = 3) in flat uint v_material_idx;

layout(push_constant) uniform PushConstants
{
    Camera camera;
};

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(set = 1, binding = 0) buffer readonly PointLightBuffer
{
    PointLight point_lights[];
};

layout(set = 1, binding = 1) buffer readonly DirectionalLightBuffer
{
    DirectionalLight directional_lights[];
};

layout(set = 1, binding = 2) buffer readonly SpotLightBuffer
{
    PointLight spot_lights[];
};

layout(set = 3, binding = 0) buffer readonly MaterialBuffer
{
    Material materials[];
};

layout(location = 0) out vec4 f_color;

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}

float DistributionGGX(vec3 N, vec3 H, float a) // Trowbridge-Reitz GGX
{
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;

    return num / denom;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 apply_light(vec3 eye, vec3 p, vec3 N, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness)
{
    vec3 V = normalize(eye - p);

    vec3 H = normalize(V + -L);

    vec3 F0 = vec3(0.04); // Base reflectivity
    F0 = mix(F0, albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, -L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float num = NDF * G;
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, -L), 0.0) + 0.0001;
    float specular = num / denom;

    float NdotL = max(dot(N, -L), 0.0);
    return max((kD * albedo / PI + kS * specular) * radiance * NdotL, 0);
}

vec3 apply_point_light(vec3 eye, vec3 p, vec3 N, PointLight point_light, vec3 albedo, float metallic, float roughness)
{
    vec3 d = p - point_light.position;
    vec3 L = normalize(d);
    float intensity = 1.0 / pow(length(d), 0.81);
    vec3 radiance = point_light.color * intensity;

    return apply_light(eye, p, N, L, radiance, albedo, metallic, roughness);
}

vec3 gamma_correction(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

void main()
{
    Material material = materials[v_material_idx];

    vec3 albedo = texture(textures[material.base_color_texture_idx], v_texcoords).rgb;
    float metallic = texture(textures[material.metallic_roughness_texture_idx], v_texcoords).b;
    float roughness = texture(textures[material.metallic_roughness_texture_idx], v_texcoords).g;

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < point_lights.length(); i++) {
        Lo += apply_point_light(camera.position, v_position, v_normal, point_lights[i], albedo, metallic, roughness);
    }

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
    color = gamma_correction(color);
    f_color = vec4(color, 1.0);
}
