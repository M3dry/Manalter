#version 460

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec3 v_frag_position;

layout (location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec4 color;
    vec4 light_color;
    vec4 light_position;
    vec4 view_position;
    vec3 info; // .x = strength, .y = shine, .z = ambient_strength
};

void main() {
    vec4 ambient = info.z * light_color;

    vec3 norm = normalize(v_normal);
    vec3 difference = vec3(light_position) - v_frag_position;
    vec3 light_dir = normalize(difference);
    float distance = length(difference);
    float diff = max(dot(norm, light_dir), 0.0);
    diff = (diff + 0.5) / 1.5;
    vec4 diffuse = diff * light_color * clamp(1.0 - (distance / 5.0f), 0.0, 1.0);

    vec3 view_dir = normalize(vec3(view_position) - v_frag_position);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), info.y);
    vec4 specular = info.x * spec * light_color;

    FragColor = vec4(vec3((ambient + diffuse + specular) * color), 1.0f);
}
