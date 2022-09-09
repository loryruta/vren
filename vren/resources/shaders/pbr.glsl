#ifndef VREN_PBR_H_
#define VREN_PBR_H_

#include <common.glsl>

float pbr_GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float pbr_GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = pbr_GeometrySchlickGGX(NdotV, k);
    float ggx2 = pbr_GeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}

float pbr_DistributionGGX(vec3 N, vec3 H, float a) // Trowbridge-Reitz GGX
{
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

vec3 pbr_fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 pbr_apply_light(
    vec3 eye,
    vec3 p,
    vec3 N,
    vec3 L,
    vec3 radiance,
    vec3 albedo,
    float metallic,
    float roughness
)
{
    vec3 V = normalize(eye - p);
    vec3 H = normalize(V + -L);

    vec3 F0 = vec3(0.04); // Base reflectivity
    F0 = mix(F0, albedo, metallic);

    float NDF = pbr_DistributionGGX(N, H, roughness);
    float G = pbr_GeometrySmith(N, V, -L, roughness);
    vec3 F = pbr_fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float num = NDF * G;
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, -L), 0.0) + 0.0001;
    float specular = num / denom;

    float NdotL = max(dot(N, -L), 0.0);
    return max((kD * albedo / PI + kS * specular) * radiance * NdotL, 0);
}

vec3 pbr_apply_point_light(
    vec3 eye,
    vec3 p,
    vec3 N,
    vec3 light_position,
    PointLight point_light,
    vec3 albedo,
    float metallic,
    float roughness
)
{
    vec3 d = p - light_position;
    vec3 L = normalize(d);

    //float intensity = 1.0 / pow(length(d), 0.81);
    float intensity = 1.0 - max(length(d) / point_light.intensity, 1.0);
    vec3 radiance = point_light.color;

    return pbr_apply_light(eye, p, N, L, radiance, albedo, metallic, roughness);
}

vec3 pbr_apply_directional_light(
    vec3 eye,
    vec3 p,
    vec3 N, 
    vec3 light_direction,
    vec3 light_color,
    vec3 albedo,
    float metallic,
    float roughness
)
{
    return pbr_apply_light(eye, p, N, light_direction, light_color, albedo, metallic, roughness);
}

vec3 pbr_gamma_correct(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

#endif // VREN_PBR_H_
