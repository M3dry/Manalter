#version 460

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D tex;

void main() {
    FragColor = texture(tex, v_uv);
    // FragColor = v_color;
    // FragColor = vec4(v_uv, 0.0, 1.0);
}
