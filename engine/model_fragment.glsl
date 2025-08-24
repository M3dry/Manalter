#version 460

layout (location = 0) in vec3 world_pos;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in flat float tangent_w;
layout (location = 4) in vec2 base_color_uv;
layout (location = 5) in vec2 metallic_roughness_uv;
layout (location = 6) in vec2 normal_uv;
layout (location = 7) in vec2 occlusion_uv;
layout (location = 8) in vec2 emissive_uv;

layout (set = 2, binding = 0) uniform sampler2D base_color_tex;
layout (set = 2, binding = 1) uniform sampler2D metallic_roughness_tex;
layout (set = 2, binding = 2) uniform sampler2D normal_tex;
layout (set = 2, binding = 3) uniform sampler2D occlusion_tex;
layout (set = 2, binding = 4) uniform sampler2D emissive_tex;

layout(std140, set = 3, binding = 0) uniform Material {
    vec4 base_color_factor;
    vec4 metallic_roughness_normal_occlusion_factors;
    vec3 emissive_factor;
};

layout(std140, set = 3, binding = 1) uniform UniformBlock {
    vec3 camera_pos;
};

layout(std140, set = 3, binding = 2) uniform Lights {
    vec3 light_pos;
    vec3 light_color;
};

layout (location = 0) out vec4 FragColor;

const float PI = 3.14159265359;

vec3 get_normal() {
    vec3 N = normalize(v_normal);
    vec3 T = normalize(tangent - N * dot(N, tangent));
    vec3 B = cross(N, T) * tangent_w;

    mat3 TBN = mat3(T, B, N);

    vec3 n_ts = texture(normal_tex, normal_uv).xyz * 2.0 - 1.0;
    n_ts.xy *= metallic_roughness_normal_occlusion_factors.z;

    n_ts.z = sqrt(max(1.0 - dot(n_ts.xy, n_ts.xy), 0.0));

    return normalize(TBN * n_ts);
    // vec3 tangent_normal = texture(normal_tex, normal_uv).xyz * 2.0 - 1.0;
    // tangent_normal.xy *= metallic_roughness_normal_occlusion_factors.z;
    // tangent_normal = normalize(tangent_normal);
    //
    // vec3 N = normalize(v_normal);
    // vec3 T = normalize(tangent.xyz - N * dot(N, tangent.xyz));
    // vec3 B = cross(N, T) * tangent.w;
    // mat3 TBN = mat3(T, B, N);
    //
    // return normalize(TBN * tangent_normal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom/denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 aces_tonemap(vec3 color) {
    mat3 m1 = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );
    mat3 m2 = mat3(
        1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );

    vec3 v = m1 * color;    
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;

    return pow(clamp(m2 * (a / b), 0.0, 1.0), vec3(1.0 / 2.2));	
}

void main() {
    vec3 albedo = (base_color_factor * texture(base_color_tex, base_color_uv)).rgb;
    float metallic = metallic_roughness_normal_occlusion_factors.x * texture(metallic_roughness_tex, metallic_roughness_uv).b;
    float roughness = metallic_roughness_normal_occlusion_factors.y * texture(metallic_roughness_tex, metallic_roughness_uv).g;
    vec3 normal = get_normal();
    float occlusion = metallic_roughness_normal_occlusion_factors.w * texture(occlusion_tex, occlusion_uv).r;
    vec3 emissive = emissive_factor * texture(emissive_tex, emissive_uv).rgb;

    vec3 view = normalize(camera_pos - world_pos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    // for (uint i = 0; i < 1; i++) {
        vec3 L = normalize(light_pos - world_pos);
        vec3 H = normalize(view + L);
        float distance = length(light_pos - world_pos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_color * attenuation;

        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, view, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, view), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, view), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
        vec3 specular = numerator/denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        float NdotL = max(dot(normal, L), 0.0);

        Lo += (kD * albedo/PI + specular) * radiance * NdotL;
    // }

    vec3 ambient = vec3(0.03) * albedo * occlusion;
    vec3 color = ambient + Lo + emissive;
    color = aces_tonemap(color);
    FragColor = vec4(color, 1.0);
}
