#include "engine/model.hpp"

#include "ecs.hpp"
#include "ecs_c.hpp"
#include "engine/buffer.hpp"
#include "engine/collisions.hpp"
#include "engine/gpu.hpp"
#include "engine/pipeline.hpp"
#include "model_c.hpp"
#include "util_c.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <utility>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

tinygltf::TinyGLTF loader;

inline constexpr auto index_size = sizeof(uint32_t);
inline constexpr auto position_size = sizeof(glm::vec3);
inline constexpr auto normal_size = sizeof(glm::vec3);
inline constexpr auto tangent_size = sizeof(glm::vec4);
inline constexpr auto texcoord_size = sizeof(glm::vec2);

std::size_t get_type_size(int type) {
    switch (type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        case TINYGLTF_TYPE_MAT2: return 2 * 2;
        case TINYGLTF_TYPE_MAT3: return 3 * 3;
        case TINYGLTF_TYPE_MAT4: return 4 * 4;
        case TINYGLTF_TYPE_MATRIX:
        case TINYGLTF_TYPE_VECTOR:
            assert(false && "No idea what to do with this one, dump the model data and have fun");
    }
}

std::size_t get_component_type_size(int component_type) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_BYTE: return sizeof(int8_t);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return sizeof(uint8_t);
        case TINYGLTF_COMPONENT_TYPE_SHORT: return sizeof(short);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return sizeof(unsigned short);
        case TINYGLTF_COMPONENT_TYPE_INT: return sizeof(int);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return sizeof(unsigned int);
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return sizeof(float);
        case TINYGLTF_COMPONENT_TYPE_DOUBLE: return sizeof(double);
    }
}

float normalize_value(std::byte* value, int component_type) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return *((uint8_t*)value) / (float)std::numeric_limits<uint8_t>::max();
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return *((uint16_t*)value) / (float)std::numeric_limits<uint16_t>::max();
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return *((uint32_t*)value) / (float)std::numeric_limits<uint32_t>::max();
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return std::max(*((int8_t*)value) / (float)std::numeric_limits<int8_t>::max(), -1.0f);
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return std::max(*((int16_t*)value) / (float)std::numeric_limits<int16_t>::max(), -1.0f);
        case TINYGLTF_COMPONENT_TYPE_INT:
            return std::max(*((int32_t*)value) / (float)std::numeric_limits<int32_t>::max(), -1.0f);
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return *(float*)value;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE: return (float)*(double*)value;
    }
}

std::tuple<array_unique_ptr<std::byte>, std::size_t, std::size_t> copy_buffer(const tinygltf::Model& model,
                                                                              int accessor_id, bool normalize = false) {
    if (accessor_id == -1) return {nullptr, 0, 0};

    auto& accessor = model.accessors[(uint64_t)accessor_id];
    auto& buffer_view = model.bufferViews[(uint64_t)accessor.bufferView];
    auto* buffer = (std::byte*)model.buffers[(uint64_t)buffer_view.buffer].data.data();

    std::size_t type_size = get_type_size(accessor.type);
    std::size_t component_type_size = get_component_type_size(accessor.componentType);
    std::size_t element_size = type_size * component_type_size;
    std::size_t buffer_stride = buffer_view.byteStride;
    if (buffer_stride == 0) buffer_stride = element_size;

    array_unique_ptr<std::byte> arr{
        (std::byte*)::operator new((normalize ? sizeof(float) * type_size : element_size) * accessor.count)};
    auto* buffer_cursor = buffer + buffer_view.byteOffset + accessor.byteOffset;

    if (normalize && accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        auto* arr_buf = (float*)arr.get();
        for (std::size_t i = 0; i < accessor.count * type_size; i++) {
            arr_buf[i] = normalize_value(buffer_cursor, accessor.componentType);
            buffer_cursor = buffer_cursor + buffer_stride;
        }
    } else {
        for (std::size_t i = 0; i < accessor.count; i++) {
            std::memcpy(arr.get() + i * element_size, buffer_cursor, element_size);
            buffer_cursor = buffer_cursor + buffer_stride;
        }
    }

    return {std::move(arr), accessor.count, normalize ? sizeof(float) * type_size : element_size};
}

namespace engine::model {
    struct Handled {
        std::vector<std::pair<int, std::vector<MeshId>>> meshes;
        std::vector<std::pair<int, std::size_t>> materials;
        std::vector<std::pair<int, std::size_t>> textures;
        std::vector<std::pair<int, std::size_t>> samplers;
        std::vector<std::pair<int, std::size_t>> sources;
    };

    struct ModelData {
        std::size_t default_material = (std::size_t)-1;
        model_materials materials;
        model_textures textures;
        std::size_t default_sampler = (std::size_t)-1;
        model_samplers samplers;
        model_sources sources;
    };

    std::size_t parse_texture(const tinygltf::Model& model, int texture_id, bool srgb, Handled& handled,
                              ModelData& out) {
        if (texture_id == -1) {
            if (out.default_sampler == (std::size_t)-1) {
                out.samplers.emplace_back(default_sampler);
            }
            out.textures.emplace_back(-1, out.default_sampler);
        }
        for (auto [id, ix] : handled.textures) {
            if (id == texture_id) {
                return ix;
            }
        }
        const auto& texture = model.textures[(uint64_t)texture_id];

        std::size_t source_ix = (std::size_t)-1;
        if (texture.source != -1) {
            for (auto [id, ix] : handled.sources) {
                if (texture.source == id) {
                    source_ix = ix;
                    goto texture_loaded;
                }
            }

            const auto& src = model.images[(uint64_t)texture.source];
            bool resize = false;
            SDL_GPUTextureFormat source_format;
            if (srgb) {
                source_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
                assert(src.component == 4);
                assert(src.bits == 8);
            } else {
                switch (src.bits) {
                    case 8:
                        switch (src.component) {
                            case 1: source_format = SDL_GPU_TEXTUREFORMAT_R8_UNORM; break;
                            case 2: source_format = SDL_GPU_TEXTUREFORMAT_R8G8_UNORM; break;
                            case 3: resize = true;
                            case 4: source_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM; break;
                            default: assert(false && "Invalid `component` size of a source");
                        }
                        break;
                    case 16:
                        switch (src.component) {
                            case 1: source_format = SDL_GPU_TEXTUREFORMAT_R16_UNORM; break;
                            case 2: source_format = SDL_GPU_TEXTUREFORMAT_R16G16_UNORM; break;
                            case 3: resize = true;
                            case 4: source_format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM; break;
                            default: assert(false && "Invalid `component` size of a source");
                        }
                        break;
                    case 32: assert(false && "Unsupported source bit depth size: 32");
                    default: assert(false && "Invalid `bits` size of a source");
                }
            }

            array_unique_ptr<std::byte> source_data{};
            uint64_t size = 0;
            if (resize) {
                size = (std::size_t)(src.width * src.height * (src.bits / 8) * 4);
                assert(src.image.size() == size);

                source_data.reset((std::byte*)::operator new(size));
                for (uint i = 0; i < (uint)(src.width * src.height); i++) {
                    auto* source = (std::byte*)src.image.data() + i * 3;
                    auto* dest = source_data.get() + i * 4;
                    dest[0] = source[0];
                    dest[1] = source[1];
                    dest[2] = source[2];
                    dest[3] = std::numeric_limits<std::byte>::max();
                }
            } else {
                size = src.image.size();
                source_data.reset((std::byte*)::operator new(src.image.size()));
                std::memcpy(source_data.get(), src.image.data(), src.image.size());
            }

            out.sources.emplace_back(std::move(source_data), (uint16_t)src.width, (uint16_t)src.height, source_format);
            source_ix = out.sources.size() - 1;
            handled.sources.emplace_back(source_ix, texture.source);
        }
    texture_loaded:
        std::size_t sampler_ix = (std::size_t)-1;
        if (texture.sampler != -1) {
            for (auto [id, ix] : handled.samplers) {
                if (texture.sampler == id) {
                    sampler_ix = ix;
                    goto sampler_loaded;
                }
            }

            const auto& sampler = model.samplers[(uint64_t)texture.sampler];
            SDL_GPUSamplerCreateInfo info = default_sampler;
            switch (sampler.magFilter) {
                case TINYGLTF_TEXTURE_FILTER_LINEAR: info.mag_filter = SDL_GPU_FILTER_LINEAR; break;
                case TINYGLTF_TEXTURE_FILTER_NEAREST: info.mag_filter = SDL_GPU_FILTER_NEAREST; break;
                default: break;
            }
            bool generate_mipmaps = true;
            switch (sampler.minFilter) {
                case TINYGLTF_TEXTURE_FILTER_LINEAR:
                    info.min_filter = SDL_GPU_FILTER_LINEAR;
                    generate_mipmaps = false;
                    break;
                case TINYGLTF_TEXTURE_FILTER_NEAREST:
                    info.min_filter = SDL_GPU_FILTER_LINEAR;
                    generate_mipmaps = false;
                    break;
                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
                    info.min_filter = SDL_GPU_FILTER_LINEAR;
                    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
                    break;
                case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
                    info.min_filter = SDL_GPU_FILTER_LINEAR;
                    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
                    break;
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
                    info.min_filter = SDL_GPU_FILTER_NEAREST;
                    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
                    break;
                case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
                    info.min_filter = SDL_GPU_FILTER_NEAREST;
                    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
                    break;
                default: break;
            }

            std::pair<int, SDL_GPUSamplerAddressMode&> wraps[2] = {{sampler.wrapS, info.address_mode_u},
                                                                   {sampler.wrapT, info.address_mode_v}};
            for (auto [wrap, address_mode] : wraps) {
                switch (wrap) {
                    case TINYGLTF_TEXTURE_WRAP_REPEAT: address_mode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT; break;
                    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
                        address_mode = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
                        break;
                    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
                        address_mode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
                        break;
                    default: break;
                }
            }

            out.samplers.emplace_back(info);
            sampler_ix = out.samplers.size() - 1;
            handled.samplers.emplace_back(sampler_ix, texture.sampler);
        } else if (out.default_sampler == (std::size_t)-1) {
            out.samplers.emplace_back(default_sampler);
            sampler_ix = out.samplers.size() - 1;
        } else {
            sampler_ix = out.default_sampler;
        }
    sampler_loaded:

        out.textures.emplace_back(source_ix, sampler_ix);
        auto ix = out.textures.size() - 1;
        handled.textures.emplace_back(texture_id, ix);
        return ix;
    }

    std::size_t parse_material(const tinygltf::Model& model, int material_id, Handled& handled, ModelData& out) {
        if (material_id == -1) {
            if (out.default_material == (std::size_t)-1) {
                out.materials.emplace_back(default_material);
            }

            return out.default_material;
        }

        for (auto [id, ix] : handled.materials) {
            if (id == material_id) {
                return ix;
            }
        }

        const auto& mat = model.materials[(uint64_t)material_id];
        Material out_mat;
        auto& pbr = mat.pbrMetallicRoughness;

        out_mat.base_color = glm::make_vec4(pbr.baseColorFactor.data());
        out_mat.base_color_texture = parse_texture(model, pbr.baseColorTexture.index, false, handled, out);
        out_mat.texcoord_mapping[(std::size_t)TexcoordKey::BaseColor] = (TexcoordMap)pbr.baseColorTexture.texCoord;

        out_mat.metallic_factor = (float)pbr.metallicFactor;
        out_mat.roughness_factor = (float)pbr.roughnessFactor;
        out_mat.metallic_roughness_texture = parse_texture(model, pbr.metallicRoughnessTexture.index, false, handled, out);
        out_mat.texcoord_mapping[(std::size_t)TexcoordKey::PBR] = (TexcoordMap)pbr.metallicRoughnessTexture.texCoord;

        out_mat.normal_factor = (float)mat.normalTexture.scale;
        out_mat.normal_texture = parse_texture(model, mat.normalTexture.index, false, handled, out);
        out_mat.texcoord_mapping[(std::size_t)TexcoordKey::Normal] = (TexcoordMap)mat.normalTexture.texCoord;

        out_mat.occlusion_factor = (float)mat.occlusionTexture.strength;
        out_mat.occlusion_texture = parse_texture(model, mat.occlusionTexture.index, false, handled, out);
        out_mat.texcoord_mapping[(std::size_t)TexcoordKey::Occlusion] = (TexcoordMap)mat.occlusionTexture.texCoord;

        out_mat.emissive_factor = glm::make_vec3(mat.emissiveFactor.data());
        out_mat.emissive_texture = parse_texture(model, mat.emissiveTexture.index, true, handled, out);
        out_mat.texcoord_mapping[(std::size_t)TexcoordKey::Emissive] = (TexcoordMap)mat.emissiveTexture.texCoord;

        out.materials.emplace_back(out_mat);
        auto ix = out.materials.size() - 1;
        handled.materials.emplace_back(material_id, ix);
        return ix;
    }

    MeshId parse_primitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
                           collisions::AABB& model_aabb, Handled& handled, ModelData& out) {
        auto it = primitive.attributes.find("POSITION");
        if (it == primitive.attributes.end()) assert(false && "GLTF doesn't have a POSITION attribute");

        auto position_accessor_id = it->second;
        auto normal_accessor_id = -1;
        auto tangent_accessor_id = -1;
        int texcoord_accessor_id[4] = {-1, -1, -1, -1};

        it = primitive.attributes.find("NORMAL");
        if (it != primitive.attributes.end()) normal_accessor_id = it->second;
        it = primitive.attributes.find("TANGENT");
        if (it != primitive.attributes.end()) tangent_accessor_id = it->second;
        for (std::size_t i = 0; i < sizeof(texcoord_accessor_id) / sizeof(int); i++) {
            it = primitive.attributes.find(std::format("TEXCOORD_{}", i));
            if (it != primitive.attributes.end()) texcoord_accessor_id[i] = it->second;
        }

        pipeline::PrimitiveType primitive_type;
        switch (primitive.mode) {
            case TINYGLTF_MODE_POINTS: primitive_type = pipeline::PointList; break;
            case TINYGLTF_MODE_LINE: primitive_type = pipeline::LineList; break;
            case TINYGLTF_MODE_LINE_STRIP: primitive_type = pipeline::LineStrip; break;
            case TINYGLTF_MODE_TRIANGLES: primitive_type = pipeline::TriangleList; break;
            case TINYGLTF_MODE_TRIANGLE_STRIP: primitive_type = pipeline::TriangleStrip; break;
            default: assert(false && "unsupported primitive type");
        }

        MeshIndices mesh_indices{};
        mesh_indices.indices = nullptr;
        if (primitive.indices != -1) {
            auto [indices_ptr, count, element_size] = copy_buffer(model, primitive.indices);

            mesh_indices.indices = std::move(indices_ptr);
            mesh_indices.count = count;
            if (element_size == sizeof(uint32_t)) {
                mesh_indices.stride = SDL_GPU_INDEXELEMENTSIZE_32BIT;
            } else if (element_size == sizeof(uint16_t)) {
                mesh_indices.stride = SDL_GPU_INDEXELEMENTSIZE_16BIT;
            } else {
                assert(false && "Invalid index buffer stride");
            }
        }

        auto [position_ptr, position_count, position_size] = copy_buffer(model, position_accessor_id);
        assert(position_size == sizeof(glm::vec3));

        auto [normal_ptr, normal_count, normal_size] = copy_buffer(model, normal_accessor_id);
        if (normal_ptr != nullptr) {
            assert(normal_count == position_count);
            assert(normal_size == sizeof(glm::vec3));
        }

        auto [tangent_ptr, tangent_count, tangent_size] = copy_buffer(model, tangent_accessor_id);
        if (tangent_ptr != nullptr) {
            assert(tangent_count == position_count);
            assert(tangent_size == sizeof(glm::vec4));
        }

        std::array<array_unique_ptr<glm::vec2>, 4> texcoord_accessor_ptr{nullptr, nullptr, nullptr, nullptr};
        for (std::size_t i = 0; i < sizeof(texcoord_accessor_id) / sizeof(int); i++) {
            auto [ptr, count, size] = copy_buffer(model, texcoord_accessor_id[i], true);
            if (ptr == nullptr) continue;

            assert(count == position_count);
            assert(size == sizeof(glm::vec2));

            texcoord_accessor_ptr[i].reset((glm::vec2*)ptr.release());
        }

        collisions::AABB aabb{};
        {
            auto& accessor = model.accessors[(uint64_t)position_accessor_id];
            aabb.min = glm::make_vec3(accessor.minValues.data());
            aabb.max = glm::make_vec3(accessor.maxValues.data());
        }

        model_aabb.extend(aabb);

        auto material_ix = parse_material(model, primitive.material, handled, out);

        MeshId mesh_id{ecs.static_emplace_entity<archetypes::Mesh>(
            std::move(mesh_indices), primitive_type, (uint32_t)position_count, (glm::vec3*)position_ptr.release(),
            (glm::vec3*)normal_ptr.release(), (glm::vec4*)tangent_ptr.release(), std::move(texcoord_accessor_ptr),
            material_ix, aabb)};
        return mesh_id;
    }

    void parse_node(const tinygltf::Model& model, int node_id, std::vector<glm::mat4>& mesh_transforms,
                    std::vector<MeshId>& meshes, glm::mat4 transform, collisions::AABB& model_aabb, Handled& handled,
                    ModelData& out) {
        if (node_id == -1) return;

        auto& node = model.nodes[(uint64_t)node_id];

        glm::mat4 mat;
        if (node.matrix.size() != 0) {
            mat = glm::make_mat4(node.matrix.data());
        } else {
            glm::vec3 scale{1.0f};
            if (node.scale.size() != 0) scale = glm::make_vec3(node.scale.data());

            glm::quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
            if (node.rotation.size() != 0) rotation = glm::make_quat(node.rotation.data());

            glm::vec3 translate{0.0f};
            if (node.translation.size() != 0) translate = glm::make_vec3(node.translation.data());

            mat = glm::translate(glm::identity<glm::mat4>(), translate) * glm::mat4_cast(rotation) *
                  glm::scale(glm::identity<glm::mat4>(), scale);
        }

        mat = mat * transform;
        if (node.mesh != -1) {
            for (auto [gltf_mesh_id, ecs_mesh_id] : handled.meshes) {
                if (gltf_mesh_id == node.mesh) {
                    meshes.insert(meshes.end(), ecs_mesh_id.begin(), ecs_mesh_id.end());
                    for (std::size_t i = 0; i < ecs_mesh_id.size(); i++) {
                        mesh_transforms.emplace_back(mat);
                    }
                    goto mesh_loaded;
                }
            }

            auto& mesh = model.meshes[(uint64_t)node.mesh];
            std::vector<MeshId> handled_meshes{};
            for (auto& primitive : mesh.primitives) {
                auto mesh_id = parse_primitive(model, primitive, model_aabb, handled, out);
                meshes.emplace_back(mesh_id);
                mesh_transforms.emplace_back(mat);
                handled_meshes.emplace_back(mesh_id);
            }
            handled.meshes.emplace_back(node.mesh, std::move(handled_meshes));
        }
    mesh_loaded:

        for (auto children_id : node.children) {
            parse_node(model, children_id, mesh_transforms, meshes, mat, model_aabb, handled, out);
        }
    }

    ModelId parse_model(const tinygltf::Model& model) {
        ModelData out{};

        int scene_id = model.defaultScene;
        if (scene_id == -1 && model.scenes.size() > 0) {
            scene_id = 0;
        }

        std::vector<MeshId> meshes;
        std::vector<glm::mat4> mesh_transforms;
        collisions::AABB aabb{};
        meshes.reserve(model.nodes.size()); // uint64_t is small and fragmentation bad
        if (scene_id != -1) {
            auto& scene = model.scenes[(uint64_t)scene_id];

            Handled handled;
            for (auto node_id : scene.nodes) {
                parse_node(model, node_id, mesh_transforms, meshes, glm::identity<glm::mat4>(), aabb, handled, out);
            }
        } else {
            assert(false && "TODO: get root nodes from model.scene array");
        }

        auto meshes_ptr = (MeshId*)::operator new(sizeof(MeshId) * meshes.size());
        auto meshe_transform_ptr = (glm::mat4*)::operator new(sizeof(glm::mat4) * meshes.size());
        std::memcpy(meshes_ptr, meshes.data(), sizeof(MeshId) * meshes.size());
        std::memcpy(meshe_transform_ptr, mesh_transforms.data(), sizeof(glm::mat4) * mesh_transforms.size());

        return ModelId{ecs.static_emplace_entity<archetypes::Model>(
            meshes.size(), meshe_transform_ptr, meshes_ptr, nullptr, aabb, std::move(out.materials),
            std::move(out.textures), std::move(out.samplers), std::move(out.sources))};
    }

    uint32_t upload_to_buffer(ModelId model_id, std::byte* buffer, uint32_t current_offset, SDL_GPUBuffer* gpu_buffer) {
        auto [mesh_count, transforms, meshes, gpu_meshes] =
            *ecs.get<archetypes::Model, model_mesh_count_tag, model_mesh_transforms_tag, model_meshes_tag,
                     model_gpu_meshes_tag>(model_id);
        gpu_meshes.reset((GPUMeshId*)::operator new(sizeof(GPUMeshId) * mesh_count));

        for (std::size_t m = 0; m < mesh_count; m++) {
            auto transform = transforms[m];
            auto [indices, vertex_count, positions, normals, tangents, texcoords] =
                *ecs.get<archetypes::Mesh, model::MeshIndices, mesh_vertex_count_tag, mesh_positions_tag,
                         mesh_normals_tag, mesh_tangents_tag, mesh_texcoords_tag>(meshes[m]);

            uint32_t attribute_offset = 0;
            uint32_t stride = 0;
            uint32_t draw_count = 0;
            GPUOffsets gpu_offsets{};
            gpu_offsets.start_offset = current_offset;
            if (indices.indices.get() != nullptr) {
                gpu_offsets.attribute_mask |= GPUOffsets::Indices;
                gpu_offsets.indices_offset = attribute_offset;
                attribute_offset += sizeof(uint32_t) * indices.count;
                draw_count = (uint32_t)indices.count;
            } else {
                draw_count = vertex_count;
            }
            if (positions.get() != nullptr) {
                gpu_offsets.attribute_mask |= GPUOffsets::Position;
                gpu_offsets.position_offset = attribute_offset;
                attribute_offset += position_size;
                stride += position_size;
            }
            if (normals.get() != nullptr) {
                gpu_offsets.attribute_mask |= GPUOffsets::Normal;
                gpu_offsets.normal_offset = attribute_offset;
                attribute_offset += normal_size;
                stride += normal_size;
            }
            if (tangents.get() != nullptr) {
                gpu_offsets.attribute_mask |= GPUOffsets::Tangent;
                gpu_offsets.tangent_offset = attribute_offset;
                attribute_offset += tangent_size;
                stride += tangent_size;
            }
            // for (auto& texcoord : texcoords) {
            //     if (texcoord.get() != nullptr) {
            //         gpu_offsets.attribute_mask |= GPUOffsets::Tangent;
            //         gpu_offsets.tangent_offset = attribute_offset;
            //         attribute_offset += tangent_size;
            //         stride += tangent_size;
            //     }
            // }
            gpu_offsets.stride = stride;

            if (gpu_offsets.attribute_mask & GPUOffsets::Indices) {
                auto* buf_view = buffer + gpu_offsets.indices_offset;
                for (std::size_t i = 0; i < indices.count; i++) {
                    if (indices.stride == SDL_GPU_INDEXELEMENTSIZE_16BIT) {
                        *(uint32_t*)buf_view = ((uint16_t*)indices.indices.get())[i];
                    } else {
                        *(uint32_t*)buf_view = ((uint32_t*)indices.indices.get())[i];
                    }

                    buf_view += sizeof(uint32_t);
                }
            }
            if (gpu_offsets.attribute_mask & GPUOffsets::Position) {
                auto* buf_view = buffer + gpu_offsets.position_offset;
                for (std::size_t i = 0; i < vertex_count; i++) {
                    *(glm::vec3*)buf_view = transform * glm::vec4{positions[i], 1.0f};

                    buf_view += gpu_offsets.stride;
                }
            }
            if (gpu_offsets.attribute_mask & GPUOffsets::Normal) {
                auto* buf_view = buffer + gpu_offsets.normal_offset;
                glm::mat3 normal_transform{glm::transpose(glm::inverse(transform))};
                for (std::size_t i = 0; i < vertex_count; i++) {
                    *(glm::vec3*)buf_view = normal_transform * normals[i];

                    buf_view += gpu_offsets.stride;
                }
            }

            current_offset += gpu_offsets.stride * vertex_count + draw_count * sizeof(uint32_t);
            gpu_meshes[m] =
                GPUMeshId{ecs.static_emplace_entity<archetypes::GPUMesh>(gpu_offsets, draw_count, gpu_buffer)};
        }

        return current_offset;
    }

    GPUGroupId new_group() {
        return GPUGroupId(ecs.static_emplace_entity<archetypes::GPUGroup>(std::vector<ModelId>{}, nullptr));
    }

    bool upload_to_gpu(GPUGroupId group_id, SDL_GPUCommandBuffer* cmd_buf) {
        auto [models, buffer] = *ecs.get<archetypes::GPUGroup>(group_id);
        if (buffer != nullptr) return false;

        uint32_t total_buffer_size = 0;
        for (auto model_id : models) {
            auto [mesh_count, meshes] = *ecs.get<archetypes::Model, model_mesh_count_tag, model_meshes_tag>(model_id);
            for (std::size_t m = 0; m < mesh_count; m++) {
                auto [indices, vertex_count, positions, normals, tangents, texcoords] =
                    *ecs.get<archetypes::Mesh, model::MeshIndices, mesh_vertex_count_tag, mesh_positions_tag,
                             mesh_normals_tag, mesh_tangents_tag, mesh_texcoords_tag>(meshes[m]);
                if (indices.indices.get() != nullptr) {
                    total_buffer_size = (uint32_t)(index_size * indices.count);
                }

                if (positions.get() != nullptr) total_buffer_size += vertex_count * position_size;
                if (normals.get() != nullptr) total_buffer_size += vertex_count * normal_size;
                if (tangents.get() != nullptr) total_buffer_size += vertex_count * tangent_size;
                for (auto& texcoord : texcoords) {
                    if (texcoord.get() == nullptr) continue;

                    total_buffer_size += vertex_count * texcoord_size;
                }
            }
        }

        buffer = Buffer{total_buffer_size,
                        static_cast<buffer::BufferUsage>(buffer::GraphicsStorageRead | buffer::Vertex | buffer::Index)}
                     .release();
        SDL_GPUTransferBufferCreateInfo transfer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = total_buffer_size,
            .props = 0,
        };
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_info);
        std::byte* data_buffer = (std::byte*)SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);

        uint32_t current_offset = 0;
        for (auto model_id : models) {
            current_offset = upload_to_buffer(model_id, data_buffer, current_offset, buffer);
        }

        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);

        SDL_GPUTransferBufferLocation src{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };
        SDL_GPUBufferRegion dest{
            .buffer = buffer,
            .offset = 0,
            .size = total_buffer_size,
        };
        SDL_UploadToGPUBuffer(copy_pass, &src, &dest, false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
        return true;
    }

    void free_from_gpu(GPUGroupId group_id) {
        auto [models, buffer] = *ecs.get<archetypes::GPUGroup>(group_id);
        SDL_ReleaseGPUBuffer(gpu_device, buffer);

        for (auto model_id : models) {
            auto [mesh_count, gpu_meshes] =
                *ecs.get<archetypes::Model, model_mesh_count_tag, model_gpu_meshes_tag>(model_id);
            for (std::size_t m = 0; m < mesh_count; m++) {
                ecs.remove(gpu_meshes[m]);
            }
        }
    }

    std::expected<ModelId, std::string> load(GPUGroupId group_id, const char* file_path, bool binary,
                                             std::string* warning) {
        auto [models, buffer] = *ecs.get<archetypes::GPUGroup>(group_id);
        if (buffer != nullptr)
            return std::unexpected(
                "GPUGroup is on the GPU, first free via `free_from_gpu`, then you can add extra models");

        tinygltf::Model model;
        std::string err;
        bool ret;

        if (binary) {
            ret = loader.LoadBinaryFromFile(&model, &err, warning, file_path);
        } else {
            ret = loader.LoadASCIIFromFile(&model, &err, warning, file_path);
        }

        if (!ret) {
            return std::unexpected(err);
        }

        auto model_id = parse_model(model);
        models.emplace_back(model_id);
        return model_id;
    }

    std::expected<ModelId, std::string> load(GPUGroupId group_id, std::span<std::byte> data, bool binary,
                                             std::string* warning) {
        auto [models, buffer] = *ecs.get<archetypes::GPUGroup>(group_id);
        if (buffer != nullptr)
            return std::unexpected(
                "GPUGroup is on the GPU, first free via `free_from_gpu`, then you can add extra models");

        tinygltf::Model model;
        std::string err;
        bool ret;

        if (binary) {
            ret = loader.LoadBinaryFromMemory(&model, &err, warning, (unsigned char*)data.data(),
                                              (unsigned int)data.size());
        } else {
            ret = loader.LoadASCIIFromString(&model, &err, warning, (const char*)(data.data()),
                                             (unsigned int)data.size(), "");
        }

        if (!ret) {
            return std::unexpected(err);
        }

        auto model_id = parse_model(model);
        models.emplace_back(model_id);
        return model_id;
    }

    void dump_data(ModelId model_id) {
        auto [mesh_count, mesh_transforms, meshes, gpu_meshes, aabb, materials, textures, samplers, sources] =
            *ecs.get<ecs::Static<archetypes::Model>>(model_id);

        std::println("AABB:\n\tmin: [{}, {}, {}]\n\tmax: [{}, {}, {}]", aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x,
                     aabb.max.y, aabb.max.z);
        std::println("mesh count: {}", mesh_count);
        for (std::size_t i = 0; i < mesh_count; i++) {
            auto [mesh_indices, mode, vertices, positions, normals, tangents, texcoords, material, mesh_aabb] =
                *ecs.get<ecs::Static<archetypes::Mesh>>(meshes[i]);

            std::println("mesh[{}]", ecs::Entity(meshes[i]));
            std::println("\tindices count: {}", mesh_indices.count);
            std::println("\tstride: {}", static_cast<int>(mesh_indices.stride));
            std::println("\tmode: {}", static_cast<int>(mode));
            std::println("\tvertices: {}", vertices);
            std::println("\tmaterial: {}", material);
            for (std::size_t t = 0; t < texcoords.size(); t++) {
                std::println("\ttexcoord[{}] = {}", t, texcoords[t].get() != nullptr);
            }
        }

        for (std::size_t m = 0; m < materials.size(); m++) {
            const auto& mat = materials[m];

            std::println("material[{}]:", m);
            std::println("\tbase color: [{}, {}, {}, {}]", mat.base_color.x, mat.base_color.y, mat.base_color.z, mat.base_color.w);
            std::println("\tbase color texture: {}", mat.base_color_texture);
            std::println("\ttexcoord[base_color]: {}", (std::size_t)mat.texcoord_mapping[(std::size_t)TexcoordKey::BaseColor]);
            std::println("\tmetallic factor: {}", mat.metallic_factor);
            std::println("\troughness factor: {}", mat.roughness_factor);
            std::println("\tmetallic roughness texture: {}", mat.metallic_roughness_texture);
            std::println("\ttexcoord[pbr]: {}", (std::size_t)mat.texcoord_mapping[(std::size_t)TexcoordKey::PBR]);
            std::println("\tnormal factor: {}", mat.normal_factor);
            std::println("\tnormal texture: {}", mat.normal_texture);
            std::println("\ttexcoord[normal]: {}", (std::size_t)mat.texcoord_mapping[(std::size_t)TexcoordKey::Normal]);
            std::println("\tocclusion factor: {}", mat.occlusion_factor);
            std::println("\tocclusion texture: {}", mat.occlusion_texture);
            std::println("\ttexcoord[occlusion]: {}", (std::size_t)mat.texcoord_mapping[(std::size_t)TexcoordKey::Occlusion]);
            std::println("\temissive factor: [{}, {}, {}]", mat.emissive_factor.x, mat.emissive_factor.y, mat.emissive_factor.z);
            std::println("\temissive texture: {}", mat.emissive_texture);
            std::println("\ttexcoord[emissive]: {}", (std::size_t)mat.texcoord_mapping[(std::size_t)TexcoordKey::Emissive]);
        }

        for (std::size_t t = 0; t < textures.size(); t++) {
            const auto& [model_source, model_sampler] = textures.get<model_source_id, model_sampler_id>(t);

            std::println("texture[{}]", t);
            std::println("\tsampler: {}", model_sampler);
            std::println("\tsource: {}", model_source);
        }

        for (std::size_t s = 0; s < samplers.size(); s++) {
            const auto& sampler = samplers[s];

            std::println("sampler[{}]", s);
            std::println("\tmag filter: {}", (int)sampler.mag_filter);
            std::println("\tmin filter: {}", (int)sampler.min_filter);
            std::println("\tmipmap mode: {}", (int)sampler.mipmap_mode);
            std::println("\twrap S(U): {}", (int)sampler.address_mode_u);
            std::println("\twrap T(V): {}", (int)sampler.address_mode_v);
        }

        for (std::size_t s = 0; s < sources.size(); s++) {
            const auto& source = sources[s];

            std::println("source[{}]", s);
            std::println("\tdata size: {}", source.size);
            std::println("\twidth: {}", source.width);
            std::println("\theight: {}", source.height);
            std::println("\tformat: {}", (int)source.format);
        }

        std::println("gpu meshes is null: {}", gpu_meshes.get() == nullptr);
        if (gpu_meshes.get() != nullptr) {
            for (std::size_t i = 0; i < mesh_count; i++) {
                auto [offsets, draw_count, buffer] = *ecs.get<archetypes::GPUMesh>(gpu_meshes[i]);
                std::println("gpu mesh id: {}", ecs::Entity(gpu_meshes[i]));
                std::println("\tvertex count: {}", draw_count);
                std::println("\tstart offset: {}", offsets.start_offset);
                std::println("\tindices offset: {}", offsets.indices_offset);
                std::println("\tposition offset: {}", offsets.position_offset);
                std::println("\tnormal offset: {}", offsets.normal_offset);
                std::println("\tstride: {}", offsets.stride);
                std::println("\tattribute_mask: {:b}", offsets.attribute_mask);
            }
        }
    }

    bool draw_model(ModelId model_id, SDL_GPUCommandBuffer* cmd_buf, SDL_GPURenderPass* render_pass,
                    uint32_t uniform_slot, uint32_t buffer_slot) {
        auto data = ecs.get<archetypes::Model, model_mesh_count_tag, model_meshes_tag, model_gpu_meshes_tag>(model_id);
        if (!data) return false;

        auto [mesh_count, cpu_meshes, gpu_meshes] = *data;

        for (std::size_t m = 0; m < mesh_count; m++) {
            auto [gpu_offsets, draw_count, buffer] = *ecs.get<archetypes::GPUMesh>(gpu_meshes[m]);

            SDL_GPUBufferBinding index_binding{
                .buffer = buffer,
                .offset = gpu_offsets.start_offset + gpu_offsets.indices_offset,
            };
            SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_GPUBufferBinding attrib_binding{
                .buffer = buffer,
                .offset = gpu_offsets.start_offset + gpu_offsets.position_offset,
            };
            SDL_BindGPUVertexBuffers(render_pass, 0, &attrib_binding, 1);
            SDL_DrawGPUIndexedPrimitives(render_pass, draw_count, 1, 0, 0, 0);
        }

        return true;
    }
}
