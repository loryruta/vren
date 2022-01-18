#version 460

#define PI 3.1415926538

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 3) in vec2 v_tex_coords;

// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#reference-material-pbrmetallicroughness
layout(set = 0, binding = 0) uniform sampler2D u_mat_base_color_tex;
layout(set = 0, binding = 1) uniform sampler2D u_mat_metallic_roughness_tex;

layout(push_constant) uniform PushConstants
{
    vec4 camera_pos;
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
    uint num;
    float _pad[3];
    PointLight data[];
} b_point_lights;

layout(set = 1, binding = 1) buffer readonly DirectionalLights
{
    uint num;
    float _pad[3];
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

vec3 apply_light(
    vec3 V,
    vec3 N,
    vec3 L,
    vec3 radiance,
    vec3 surf_col
)
{
    float roughness = texture(u_mat_metallic_roughness_tex, v_tex_coords).g;
    float metallic = texture(u_mat_metallic_roughness_tex,  v_tex_coords).b;

    vec3 H = normalize(V + -L);

    vec3 F0 = vec3(0.04); // Base reflectivity
    F0 = mix(F0, surf_col, metallic);

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
    return (kD * surf_col / PI + kS * specular) * radiance * NdotL;
}

vec3 apply_point_lights(
    vec3 frag_pos,
    vec3 V,
    vec3 N,
    vec3 surf_col
)
{
    vec3 Lo = vec3(0);

    for (int i = 0; i < b_point_lights.num; i++)
    {
        PointLight light = b_point_lights.data[i];

        vec3 L = normalize(frag_pos - light.position.xyz);
        vec3 radiance = light.color.rgb;

        Lo += apply_light(
            V,
            N,
            L,
            radiance,
            surf_col
        );
    }

    return Lo;
}

vec3 apply_directional_lights(
    vec3 frag_pos,
    vec3 V,
    vec3 N,
    vec3 surf_col
)
{
    vec3 Lo = vec3(0);

    for (int i = 0; i < b_directional_lights.num; i++)
    {
        DirectionalLight light = b_directional_lights.data[i];

        vec3 L = light.direction.xyz;
        vec3 radiance = light.color.rgb;

        Lo += apply_light(
            V,
            N,
            L,
            radiance,
            surf_col
        );
    }

    return Lo;
}

void main()
{
    vec3 frag_pos = v_position;
    vec3 N = v_normal;
    vec3 V = normalize(push_constants.camera_pos.xyz - frag_pos);

    vec3 albedo = texture(u_mat_base_color_tex, v_tex_coords).rgb;

    vec3 Lo = vec3(0.0);
    Lo += apply_point_lights(frag_pos, V, N, albedo);
    //Lo += apply_directional_lights(frag_pos, V, N, albedo);

    vec3 ambient = vec3(0.03) * albedo; // * ao
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2)); // Gamma correction

    f_color = vec4(color, 1.0);
}
