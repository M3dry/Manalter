#version 460

layout (location = 0) in vec2 base_color_uv;
layout (location = 1) in vec2 metallic_roughness_uv;
layout (location = 2) in vec2 normal_uv;
layout (location = 3) in vec2 occlusion_uv;
layout (location = 4) in vec2 emissive_uv;

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

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(base_color_uv, 0.0, 1.0);
}
