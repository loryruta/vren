#version 460

#extension GL_EXT_scalar_block_layout : require

#define PI 3.1415926538

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 3) in vec2 v_tex_coords;

layout(set = 0, binding = 0) uniform sampler2D u_albedo;
layout(set = 0, binding = 1) uniform MaterialBlock
{
    float metallic;
    float roughness;
} u_material;

layout(push_constant) uniform PushConstants
{
    mat4 camera_view;
    mat4 camera_projection;
} push_constants;

struct PointLight
{
    vec4 position;
    vec4 color;
};

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};

layout(set = 1, binding = 0) buffer readonly PointLights
{
    PointLight data[];
} b_point_lights;

layout(set = 1, binding = 1) buffer readonly DirectionalLights
{
    DirectionalLight data[];
} b_directional_lights;

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

void main()
{
    vec3 N = v_normal;
    vec3 camera_position = push_constants.camera_view[3].xyz;
    vec3 V = normalize(camera_position - v_position);

    vec3 albedo = texture(u_albedo, v_tex_coords).rgb;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, u_material.metallic);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < b_point_lights.data.length(); i++)
    {
        PointLight light = b_point_lights.data[i];

        vec3 radiance = light.color.rgb;

        vec3 L = normalize(v_position - light.position.xyz);
        vec3 H = normalize(V + L);

        float NDF = DistributionGGX(N, H, u_material.roughness);
        float G   = GeometrySmith(N, V, L, u_material.roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - u_material.metallic;

        vec3  num   = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = num / denom;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo; // * ao
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    f_color = vec4(color, 1.0);
}
