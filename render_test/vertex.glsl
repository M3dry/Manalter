#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 0) out vec4 v_color;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 mvp;
};

void main()
{
    gl_Position = mvp * vec4(a_position, 1.0f);
    v_color = a_color;
}
