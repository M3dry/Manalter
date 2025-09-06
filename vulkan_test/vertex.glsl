#version 460

#extension GL_EXT_buffer_reference : require

layout(buffer_reference, std430) readonly buffer Vertices {
    vec4 data[];
};

layout(push_constant, std430) uniform Push {
    Vertices verts;
};

void main() {
    gl_Position = verts.data[gl_VertexIndex % 3];
}
