// #version 460
//
// layout(std140, set = 1, binding = 0) uniform Offsets {
//     // start offset
//     // indices offset
//     // position offset
//     // normal offset
//     uvec4 offsets1;
//     // tangent offset
//     // base color coord offset
//     // mettalic roughness coord offset
//     // normal coord offset
//     uvec4 offsets2;
//     // occlusion coord offset
//     // emissive coord offset
//     // stride
//     // attribute mask
//     uvec4 offsets3;
// };
//
// layout(std140, set = 1, binding = 1) uniform UniformBlock {
//     mat4 mvp;
// };
//
// layout(std430, set = 0, binding = 0) buffer VertexData {
//     uint raw[];
// };
//
// layout(location = 0) out vec2 base_color_uv;
// layout(location = 1) out vec2 metallic_roughness_uv;
// layout(location = 2) out vec2 normal_uv;
// layout(location = 3) out vec2 occlusion_uv;
// layout(location = 4) out vec2 emissive_uv;
//
// const uint Indices = 1 << 0;
// const uint Position = 1 << 1;
// const uint Normal = 1 << 2;
// const uint Tangent = 1 << 3;
// const uint BaseColorCoord = 1 << 4;
// const uint MetallicRoughnessCoord = 1 << 5;
// const uint NormalCoord = 1 << 6;
// const uint OcclusionCoord = 1 << 7;
// const uint EmissiveCoord = 1 << 8;
//
// bool has(uint flag) {
//     return (offsets3.w & flag) != 0;
// }
//
// struct Attribute {
//     uint vertex_index;
//     vec3 position;
//     vec3 normal;
//     vec4 tangent;
//     vec2 base_color_uv;
//     vec2 metallic_roughness_uv;
//     vec2 normal_uv;
//     vec2 occlusion_uv;
//     vec2 emissive_uv;
// };
//
// Attribute init_attributes() {
//     Attribute attrib;
//     uint start_offset = offsets1.x;
//     uint stride = offsets3.z;
//
//     if (has(Indices)) {
//         attrib.vertex_index = raw[start_offset + offsets1.y / 4 + gl_VertexIndex];
//     } else {
//         attrib.vertex_index = gl_VertexIndex;
//     }
//
//     uint base_ix = start_offset + stride * attrib.vertex_index;
//     if (has(Position)) {
//         uint v_ix = base_ix + offsets1.z / 4;
//         attrib.position = vec3(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1), uintBitsToFloat(v_ix + 2));
//     } else {
//         attrib.position = vec3(0.0);
//     }
//
//     if (has(Normal)) {
//         uint v_ix = base_ix + offsets1.w / 4;
//         attrib.normal = vec3(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1), uintBitsToFloat(v_ix + 2));
//     } else {
//         attrib.normal = vec3(0.0);
//     }
//
//     if (has(Tangent)) {
//         uint v_ix = base_ix + offsets2.x / 4;
//         attrib.tangent = vec4(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1), uintBitsToFloat(v_ix + 2), uintBitsToFloat(v_ix + 3));
//     } else {
//         attrib.tangent = vec4(0.0);
//     }
//
//     if (has(BaseColorCoord)) {
//         uint v_ix = base_ix + offsets2.y / 4;
//         attrib.base_color_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.base_color_uv = vec2(0.0);
//     }
//
//     if (has(MetallicRoughnessCoord)) {
//         uint v_ix = base_ix + offsets2.z / 4;
//         attrib.metallic_roughness_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.metallic_roughness_uv = vec2(0.0);
//     }
//
//     if (has(NormalCoord)) {
//         uint v_ix = base_ix + offsets2.w / 4;
//         attrib.normal_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.normal_uv = vec2(0.0);
//     }
//
//     if (has(OcclusionCoord)) {
//         uint v_ix = base_ix + offsets3.x / 4;
//         attrib.occlusion_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.occlusion_uv = vec2(0.0);
//     }
//
//     if (has(EmissiveCoord)) {
//         uint v_ix = base_ix + offsets3.y / 4;
//         attrib.emissive_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.emissive_uv = vec2(0.0);
//     }
//
//     return attrib;
// }
//
// void main() {
//     Attribute attrib = init_attributes();
//
//     gl_Position = mvp * vec4(attrib.position, 1.0);
//     base_color_uv = attrib.base_color_uv;
//     metallic_roughness_uv = attrib.metallic_roughness_uv;
//     normal_uv = attrib.normal_uv;
//     occlusion_uv = attrib.occlusion_uv;
//     emissive_uv = attrib.emissive_uv;
// }
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

layout (location = 0) out vec2 base_color_uv;
layout (location = 1) out vec2 metalic_roughness_uv;
layout (location = 2) out vec2 normal_uv;
layout (location = 3) out vec2 occlusion_uv;
layout (location = 4) out vec2 emissive_uv;

bool has(uint flag) {
    return (offsets3.w & flag) != 0;
}

vec2 read_vec2(uint starting_offset, uint vertex_index) {
    uint base_ix = offsets1.x/4 + starting_offset/4 + vertex_index*offsets3.z/4;

    return vec2(uintBitsToFloat(raw[base_ix]), uintBitsToFloat(raw[base_ix + 1]));
}

vec3 read_vec3(uint starting_offset, uint vertex_index) {
    uint base_ix = offsets1.x/4 + starting_offset/4 + vertex_index*offsets3.z/4;

    return vec3(uintBitsToFloat(raw[base_ix]), uintBitsToFloat(raw[base_ix + 1]), uintBitsToFloat(raw[base_ix + 2]));
}

// struct Attribute {
//     uint vertex_index;
//     vec3 position;
//     vec3 normal;
//     vec4 tangent;
//     vec2 base_color_uv;
//     vec2 metallic_roughness_uv;
//     vec2 normal_uv;
//     vec2 occlusion_uv;
//     vec2 emissive_uv;
// };
//
// Attribute init_attributes() {
//     Attribute attrib;
//     uint start_offset = offsets1.x;
//     uint stride = offsets3.z;
//
//     if (has(Indices)) {
//         attrib.vertex_index = raw[start_offset + offsets1.y / 4 + gl_VertexIndex];
//     } else {
//         attrib.vertex_index = gl_VertexIndex;
//     }
//
//     uint base_ix = start_offset + stride * attrib.vertex_index;
//     if (has(Position)) {
//         uint v_ix = base_ix + offsets1.z / 4;
//         attrib.position = vec3(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1), uintBitsToFloat(v_ix + 2));
//     } else {
//         attrib.position = vec3(0.0);
//     }
//
//     if (has(Normal)) {
//         uint v_ix = base_ix + offsets1.w / 4;
//         attrib.normal = vec3(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1), uintBitsToFloat(v_ix + 2));
//     } else {
//         attrib.normal = vec3(0.0);
//     }
//
//     if (has(Tangent)) {
//         uint v_ix = base_ix + offsets2.x / 4;
//         attrib.tangent = vec4(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1), uintBitsToFloat(v_ix + 2), uintBitsToFloat(v_ix + 3));
//     } else {
//         attrib.tangent = vec4(0.0);
//     }
//
//     if (has(BaseColorCoord)) {
//         uint v_ix = base_ix + offsets2.y / 4;
//         attrib.base_color_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.base_color_uv = vec2(0.0);
//     }
//
//     if (has(MetallicRoughnessCoord)) {
//         uint v_ix = base_ix + offsets2.z / 4;
//         attrib.metallic_roughness_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.metallic_roughness_uv = vec2(0.0);
//     }
//
//     if (has(NormalCoord)) {
//         uint v_ix = base_ix + offsets2.w / 4;
//         attrib.normal_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.normal_uv = vec2(0.0);
//     }
//
//     if (has(OcclusionCoord)) {
//         uint v_ix = base_ix + offsets3.x / 4;
//         attrib.occlusion_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.occlusion_uv = vec2(0.0);
//     }
//
//     if (has(EmissiveCoord)) {
//         uint v_ix = base_ix + offsets3.y / 4;
//         attrib.emissive_uv = vec2(uintBitsToFloat(v_ix), uintBitsToFloat(v_ix + 1));
//     } else {
//         attrib.emissive_uv = vec2(0.0);
//     }
//
//     return attrib;
// }

void main() {
    uint start_offset = offsets1.x;
    uint indices_offset = offsets1.y;
    uint position_offset = offsets1.z;
    uint normal_offset = offsets1.w;
    uint tangent_offset = offsets2.x;
    uint base_color_coord_offset = offsets2.y;
    uint metallic_roughness_coord_offset = offsets2.z;
    uint normal_coord_offset = offsets2.w;
    uint occlusion_coord_offset = offsets3.x;
    uint emissive_coord_offset = offsets3.y;
    uint stride = offsets3.z;

    uint vertex_index = gl_VertexIndex;
    if (has(Indices)) {
        vertex_index = raw[start_offset/4 + indices_offset/4 + vertex_index];
    }

    vec3 position = read_vec3(position_offset, vertex_index);

    gl_Position = mvp * vec4(position, 1.0);
    base_color_uv = has(BaseColorCoord) ? read_vec2(base_color_coord_offset, vertex_index) : vec2(0.0, 0.0);
    metalic_roughness_uv = has(MetallicRoughnessCoord) ? read_vec2(metallic_roughness_coord_offset, vertex_index) : vec2(0.0, 0.0);
    normal_uv = has(NormalCoord) ? read_vec2(normal_coord_offset, vertex_index) : vec2(0.0, 0.0);
    occlusion_uv = has(OcclusionCoord) ? read_vec2(occlusion_coord_offset, vertex_index) : vec2(0.0, 0.0);
    emissive_uv = has(EmissiveCoord) ? read_vec2(emissive_coord_offset, vertex_index) : vec2(0.0, 0.0);
}
