#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in float size;
layout (location = 2) in vec4 color;

uniform mat4 mvp;

out vec4 fragColor;

void main() {
    gl_Position = mvp * vec4(pos, 1.0);
    gl_PointSize = size;
    fragColor = color;
}
