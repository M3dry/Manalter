#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec3 v_frag_position;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 vp;
    mat4 m;
};

void main() {
    gl_Position = vp * m * vec4(a_position, 1.0f);

    mat3 normal_matrix = transpose(inverse(mat3(m)));
    v_normal = normalize(normal_matrix * a_normal);

    v_frag_position = vec3(m * vec4(a_position, 1.0f));
}
