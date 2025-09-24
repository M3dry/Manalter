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
    mat4 vp;
    mat4 m;
    uint instance_offset;
};

layout(std430, set = 0, binding = 0) buffer VertexData {
    uint raw[];
};

layout(std430, set = 0, binding = 1) buffer ModelTransforms {
    mat4 model_transforms[];
};

layout (location = 0) out vec3 world_pos;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 tangent;
layout (location = 3) out flat float tangent_w;
layout (location = 4) out vec2 base_color_uv;
layout (location = 5) out vec2 metallic_roughness_uv;
layout (location = 6) out vec2 normal_uv;
layout (location = 7) out vec2 occlusion_uv;
layout (location = 8) out vec2 emissive_uv;

bool has(uint flag) {
    return (offsets3.w & flag) != 0;
}

struct Attribute {
    uint vertex_index;
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 base_color_uv;
    vec2 metallic_roughness_uv;
    vec2 normal_uv;
    vec2 occlusion_uv;
    vec2 emissive_uv;
};

Attribute init_attributes() {
    Attribute attrib;
    uint start_offset = offsets1.x/4;
    uint stride = offsets3.z/4;

    if (has(Indices)) {
        attrib.vertex_index = raw[start_offset + offsets1.y / 4 + gl_VertexIndex];
    } else {
        attrib.vertex_index = gl_VertexIndex;
    }

    uint base_ix = start_offset + stride * attrib.vertex_index;
    if (has(Position)) {
        uint v_ix = base_ix + offsets1.z / 4;
        attrib.position = vec3(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]), uintBitsToFloat(raw[v_ix + 2]));
    } else {
        attrib.position = vec3(0.0);
    }

    if (has(Normal)) {
        uint v_ix = base_ix + offsets1.w / 4;
        attrib.normal = vec3(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]), uintBitsToFloat(raw[v_ix + 2]));

        if (any(isnan(attrib.normal))) {
            attrib.normal = normalize(vec3(0.5));
        }
        if (any(isinf(attrib.normal))) {
            attrib.normal = normalize(vec3(0.5));
        }
    } else {
        attrib.normal = vec3(0.0);
    }

    if (has(Tangent)) {
        uint v_ix;
        if (has(Indices)) {
            v_ix = start_offset + offsets2.x + 4*gl_VertexIndex;
        } else {
            v_ix = base_ix + offsets2.x / 4;
        }
        attrib.tangent = vec4(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]), uintBitsToFloat(raw[v_ix + 2]), uintBitsToFloat(raw[v_ix + 3]));
    }  else {
        attrib.tangent = vec4(0.0);
    }

    if (has(BaseColorCoord)) {
        uint v_ix = base_ix + offsets2.y / 4;
        attrib.base_color_uv = vec2(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]));
    } else {
        attrib.base_color_uv = vec2(0.0);
    }

    if (has(MetallicRoughnessCoord)) {
        uint v_ix = base_ix + offsets2.z / 4;
        attrib.metallic_roughness_uv = vec2(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]));
    } else {
        attrib.metallic_roughness_uv = vec2(0.0);
    }

    if (has(NormalCoord)) {
        uint v_ix = base_ix + offsets2.w / 4;
        attrib.normal_uv = vec2(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]));
    } else {
        attrib.normal_uv = vec2(0.0);
    }

    if (has(OcclusionCoord)) {
        uint v_ix = base_ix + offsets3.x / 4;
        attrib.occlusion_uv = vec2(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]));
    } else {
        attrib.occlusion_uv = vec2(0.0);
    }

    if (has(EmissiveCoord)) {
        uint v_ix = base_ix + offsets3.y / 4;
        attrib.emissive_uv = vec2(uintBitsToFloat(raw[v_ix]), uintBitsToFloat(raw[v_ix + 1]));
    } else {
        attrib.emissive_uv = vec2(0.0);
    }

    return attrib;
}

mat4 get_model() {
    return model_transforms[instance_offset + gl_InstanceIndex] * m;
}

void main() {
    Attribute attrib = init_attributes();
    mat4 model = get_model();
    mat3 normal_matrix = transpose(inverse(mat3(model)));

    world_pos = vec3(model * vec4(attrib.position, 1.0));
    normal = normalize(normal_matrix * attrib.normal);
    tangent = mat3(model) * attrib.tangent.xyz;
    tangent_w = attrib.tangent.w;
    base_color_uv = attrib.base_color_uv;
    metallic_roughness_uv = attrib.metallic_roughness_uv;
    normal_uv = attrib.normal_uv;
    occlusion_uv = attrib.occlusion_uv;
    emissive_uv = attrib.emissive_uv;

    gl_Position = vp * vec4(world_pos, 1.0);
}
