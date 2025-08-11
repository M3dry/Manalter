#version 460

layout (location = 0) out vec3 v_color;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 mvp;
};

layout(std140, set = 1, binding = 1) uniform Offsets {
    // uint start_offset;
    // uint indices_offset;
    // uint position_offset;
    // uint normal_offset;
    uvec4 offsets1;
    // uint stride;
    // uint attribute_mask;
    uvec2 offsets2;
};

layout(std430, set = 0, binding = 0) buffer VertexData {
    uint raw[];
};

void main() {
    uint start_offset = offsets1.x;
    uint indices_offset = offsets1.y;
    uint position_offset = offsets1.z;
    uint normal_offset = offsets1.w;
    uint stride = offsets2.x;
    uint attribute_mask = offsets2.y;

    uint vertex_index = gl_VertexIndex;
    if ((attribute_mask & (1 << 0)) != 0) {
        vertex_index = raw[start_offset/4 + indices_offset/4 + vertex_index];
    }

    uint base_position_index = start_offset/4 + position_offset/4 + vertex_index * stride/4;
    float x = uintBitsToFloat(raw[base_position_index]);
    float y = uintBitsToFloat(raw[base_position_index + 1]);
    float z = uintBitsToFloat(raw[base_position_index + 2]);
    vec3 position = vec3(x, y, z);

    gl_Position = mvp * vec4(position, 1.0);
    if ((vertex_index % 3) == 0) {
        v_color = vec3(1.0, 0.0, 0.0);
    } else if ((vertex_index % 3) == 1) {
        v_color = vec3(0.0, 1.0, 0.0);
    } else if ((vertex_index % 3) == 2) {
        v_color = vec3(0.0, 0.0, 1.0);
    } else {
        v_color = vec3(1.0, 1.0, 1.0);
    }
}
