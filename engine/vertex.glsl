#version 460

#extension GL_EXT_buffer_reference : require

layout(buffer_reference, std430) readonly buffer Vertices {
    vec4 data[];
};

layout(push_constant, std430) uniform Push {
    Vertices verts;
};

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = verts.data[gl_VertexIndex % 3];
    if (gl_VertexIndex == 0) {
        uv = vec2(0.5, 1.0);
    } else if (gl_VertexIndex == 1) {
        uv = vec2(1.0, 0.0);
    } else {
        uv = vec2(0.0, 0.0);
    }
}
