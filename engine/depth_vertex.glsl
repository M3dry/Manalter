#version 460

layout(std140, set = 1, binding = 0) uniform Offsets {
    // start offset
    // indices offset
    // position offset
    // normal offset
    uvec4 offsets1;
    // tangent offset
    // base color coord offset
    // mettalic roughness coord offset
    // normal coord offset
    uvec4 offsets2;
    // occlusion coord offset
    // emissive coord offset
    // stride
    // attribute mask
    uvec4 offsets3;
};

const uint Indices = 1 << 0;
const uint Position = 1 << 1;
const uint Normal = 1 << 2;
const uint Tangent = 1 << 3;
const uint BaseColorCoord = 1 << 4;
const uint MetallicRoughnessCoord = 1 << 5;
const uint NormalCoord = 1 << 6;
const uint OcclusionCoord = 1 << 7;
const uint EmissiveCoord = 1 << 8;

layout(std140, set = 1, binding = 1) uniform UniformBlock {
    mat4 mvp;
};

layout(std430, set = 0, binding = 0) buffer VertexData {
    uint raw[];
};

bool has(uint flag) {
    return (offsets3.w & flag) != 0;
}

void main() {
    uint vertex_index;
    if (has(Indices)) {
        vertex_index = raw[offsets1.x/4 + offsets1.y/4 + gl_VertexIndex];
    } else {
        vertex_index = gl_VertexIndex;
    }

    uint position_ix = offsets1.x/4 + offsets1.z/4 + vertex_index * offsets3.z/4;

    gl_Position = mvp * vec4(uintBitsToFloat(raw[position_ix]), uintBitsToFloat(raw[position_ix + 1]), uintBitsToFloat(raw[position_ix + 2]), 1.0);
}
