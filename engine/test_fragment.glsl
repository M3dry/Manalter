#version 460

layout (location = 0) in vec4 v_color;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(vec3(1.0f) - v_color.xyz, 1.0f);
}
