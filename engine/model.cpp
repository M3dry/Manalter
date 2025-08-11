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

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

tinygltf::TinyGLTF loader;

namespace engine::model {
    std::size_t get_element_size(int type, int component_type) {
        std::size_t container_mult;
        switch (type) {
            case TINYGLTF_TYPE_SCALAR: container_mult = 1; break;
            case TINYGLTF_TYPE_VEC2: container_mult = 2; break;
            case TINYGLTF_TYPE_VEC3: container_mult = 3; break;
            case TINYGLTF_TYPE_VEC4: container_mult = 4; break;
            case TINYGLTF_TYPE_MAT2: container_mult = 2 * 2; break;
            case TINYGLTF_TYPE_MAT3: container_mult = 3 * 3; break;
            case TINYGLTF_TYPE_MAT4: container_mult = 4 * 4; break;
            case TINYGLTF_TYPE_MATRIX:
            case TINYGLTF_TYPE_VECTOR:
                assert(false && "No idea what to do with this one, dump the model data and have fun");
        }

        std::size_t component_size;
        switch (component_type) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: component_size = sizeof(std::byte); break;
            case TINYGLTF_COMPONENT_TYPE_SHORT: component_size = sizeof(short); break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: component_size = sizeof(unsigned short); break;
            case TINYGLTF_COMPONENT_TYPE_INT: component_size = sizeof(int); break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: component_size = sizeof(unsigned int); break;
            case TINYGLTF_COMPONENT_TYPE_FLOAT: component_size = sizeof(float); break;
            case TINYGLTF_COMPONENT_TYPE_DOUBLE: component_size = sizeof(double); break;
        }

        return container_mult * component_size;
    }

    std::tuple<array_unique_ptr<std::byte>, std::size_t, std::size_t> copy_buffer(const tinygltf::Model& model,
                                                                                  int accessor_id) {
        if (accessor_id == -1) return {nullptr, 0, 0};

        auto& accessor = model.accessors[(uint64_t)accessor_id];
        auto& buffer_view = model.bufferViews[(uint64_t)accessor.bufferView];
        auto* buffer = (std::byte*)model.buffers[(uint64_t)buffer_view.buffer].data.data();

        std::size_t element_size = get_element_size(accessor.type, accessor.componentType);
        std::size_t buffer_stride = buffer_view.byteStride;
        if (buffer_stride == 0) buffer_stride = element_size;

        array_unique_ptr<std::byte> arr{(std::byte*)::operator new(element_size * accessor.count)};
        auto* buffer_cursor = buffer + buffer_view.byteOffset + accessor.byteOffset;
        for (std::size_t i = 0; i < accessor.count; i++) {
            std::memcpy(arr.get() + i * element_size, buffer_cursor, element_size);
            buffer_cursor = buffer_cursor + buffer_stride;
        }

        return {std::move(arr), accessor.count, element_size};
    }

    void parse_node(const tinygltf::Model& model, int node_id, std::vector<MeshId>& meshes, glm::mat4 transform,
                    collisions::AABB& model_aabb) {
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
            auto& mesh = model.meshes[(uint64_t)node.mesh];
            for (auto& primitive : mesh.primitives) {
                auto it = primitive.attributes.find("POSITION");
                if (it == primitive.attributes.end()) assert(false && "GLTF doesn't have a POSITION attribute");

                auto position_accessor_id = it->second;
                auto normal_accessor_id = -1;

                it = primitive.attributes.find("NORMAL");
                if (it != primitive.attributes.end()) normal_accessor_id = it->second;

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
                if (primitive.indices != -1) {
                    auto [indices_ptr, count, element_size] = copy_buffer(model, primitive.indices);
                    std::print("indices ptr shit: ");
                    for (std::size_t i = 0; i < count; i++) {
                        if (element_size == sizeof(uint32_t)) {
                            std::print(" {}", ((uint32_t*)indices_ptr.get())[i]);
                        } else {
                            std::print(" {}", ((uint16_t*)indices_ptr.get())[i]);
                        }
                    }
                    std::println("");

                    mesh_indices.indices = std::move(indices_ptr);
                    mesh_indices.count = count;
                    if (element_size == sizeof(uint32_t)) {
                        mesh_indices.stride = SDL_GPU_INDEXELEMENTSIZE_32BIT;
                    } else if (element_size == sizeof(uint16_t)) {
                        mesh_indices.stride = SDL_GPU_INDEXELEMENTSIZE_16BIT;
                    } else {
                        assert(false && "Invalid index buffer stride");
                    }
                } else {
                    mesh_indices.stride = static_cast<SDL_GPUIndexElementSize>(-1);
                }

                auto [position_ptr, position_count, position_size] = copy_buffer(model, position_accessor_id);
                assert(position_size == sizeof(glm::vec3));

                auto [normal_ptr, normal_count, normal_size] = copy_buffer(model, normal_accessor_id);
                if (normal_ptr != nullptr) {
                    assert(normal_count == position_count);
                    assert(normal_size == sizeof(glm::vec3));
                }

                collisions::AABB aabb{};
                {
                    auto& accessor = model.accessors[(uint64_t)position_accessor_id];
                    aabb.min = glm::make_vec3(accessor.minValues.data());
                    aabb.max = glm::make_vec3(accessor.maxValues.data());
                }

                model_aabb.extend(aabb);

                MeshId mesh_id{ecs.static_emplace_entity<archetypes::Mesh>(
                    mat, std::move(mesh_indices), primitive_type, (uint32_t)position_count,
                    (glm::vec3*)position_ptr.release(), (glm::vec3*)normal_ptr.release(), aabb)};
                ecs.mark_dirty(mesh_id);
                meshes.emplace_back(mesh_id);
            }
        }

        for (auto children_id : node.children) {
            parse_node(model, children_id, meshes, mat, model_aabb);
        }
    }

    ModelId parse_model(const tinygltf::Model& model) {
        int scene_id = model.defaultScene;
        if (scene_id == -1 && model.scenes.size() > 0) {
            scene_id = 0;
        }

        std::vector<MeshId> meshes;
        collisions::AABB aabb{};
        meshes.reserve(model.nodes.size()); // uint64_t is small and fragmentation bad
        if (scene_id != -1) {
            auto& scene = model.scenes[(uint64_t)scene_id];

            for (auto node_id : scene.nodes) {
                parse_node(model, node_id, meshes, glm::identity<glm::mat4>(), aabb);
            }
        } else {
            assert(false && "TODO: get root nodes from model.scene array");
        }

        auto meshes_ptr = (MeshId*)::operator new(sizeof(MeshId) * meshes.size());
        std::memcpy(meshes_ptr, meshes.data(), sizeof(MeshId) * meshes.size());
        ModelId model_id{ecs.static_emplace_entity<archetypes::Model>(meshes.size(), meshes_ptr, nullptr, aabb)};

        ecs.mark_dirty(model_id);
        return model_id;
    }

    std::expected<ModelId, std::string> load(const char* file_path, bool binary, std::string* warning) {
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

        return parse_model(model);
    }

    std::expected<ModelId, std::string> load(std::span<std::byte> data, bool binary, std::string* warning) {
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

        return parse_model(model);
    }

    void dump_data(ModelId model_id) {
        auto [mesh_count, meshes, gpu_meshes, aabb] = *ecs.get<ecs::Static<archetypes::Model>>(model_id);

        std::println("AABB:\n\tmin: [{}, {}, {}]\n\tmax: [{}, {}, {}]", aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x,
                     aabb.max.y, aabb.max.z);
        std::println("mesh count: {}", mesh_count);
        for (std::size_t i = 0; i < mesh_count; i++) {
            auto [transform, mesh_indices, mode, vertices, positions, normals, mesh_aabb] =
                *ecs.get<ecs::Static<archetypes::Mesh>>(meshes[i]);

            std::println("mesh id: {}", ecs::Entity(meshes[i]));
            std::println("\tindices count: {}", mesh_indices.count);
            std::println("\tstride: {}", static_cast<int>(mesh_indices.stride));
            std::println("\tmode: {}", static_cast<int>(mode));
            std::println("\tvetices: {}", vertices);
        }
        std::println("gpu meshes is null: {}", gpu_meshes.get() == nullptr);
        for (std::size_t i = 0; i < mesh_count; i++) {
            auto [offsets, storage_buffer] = *ecs.get<archetypes::GPUMesh>(gpu_meshes[i]);
            std::println("gpu mesh id: {}", ecs::Entity(gpu_meshes[i]));
            std::println("\tstorage buffer id: {}", ecs::Entity(storage_buffer));
            std::println("\tstart offset: {}", offsets.start_offset);
            std::println("\tindices offset: {}", offsets.indices_offset);
            std::println("\tposition offset: {}", offsets.position_offset);
            std::println("\tnormal offset: {}", offsets.normal_offset);
            std::println("\tstride: {}", offsets.stride);
            std::println("\tattribute_mask: {:b}", offsets.attribute_mask);
        }
    }

    void upload_to_gpu(SDL_GPUCommandBuffer* cmd_buf) {
        constexpr auto index_size = sizeof(uint32_t);
        constexpr auto position_size = sizeof(glm::vec3);
        constexpr auto normal_size = sizeof(glm::vec3);

        auto dirty_models =
            ecs.get_dirty<archetypes::Model, model_mesh_count_tag, model_gpu_meshes_tag, model_meshes_tag>();
        auto dirty_meshes = ecs.get_dirty<archetypes::Mesh, model::MeshIndices, mesh_vertex_count_tag,
                                          mesh_positions_tag, mesh_normals_tag>();

        uint32_t total_buffer_size = 0;
        for (const auto& [indices, vertex_count, positions, normals] : dirty_meshes) {
            if (static_cast<int>(indices.stride) != -1) {
                total_buffer_size = (uint32_t)(index_size * indices.count); // indices get converted to 32bit
            }

            if (positions.get() != nullptr) total_buffer_size += vertex_count * position_size;
            if (normals.get() != nullptr) total_buffer_size += vertex_count * normal_size;
        }

        Buffer storage_buffer{total_buffer_size, static_cast<buffer::BufferUsage>(buffer::GraphicsStorageRead |
                                                                                  buffer::Vertex | buffer::Index)};
        SDL_GPUBuffer* gpu_buffer = storage_buffer; // should probably do shared_ptr stuff in `Buffer`, but currently
                                                    // it's more like unique_ptr so I just leak the buffer here, fuck it
        MeshStorageBufId mesh_strorage_buf_id{
            ecs.static_emplace_entity<archetypes::MeshStorage>(std::move(storage_buffer))};

        SDL_GPUTransferBufferCreateInfo transfer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = total_buffer_size,
            .props = 0,
        };
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_info);
        std::byte* buffer = (std::byte*)SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);

        uint32_t offset = 0;
        for (auto& [mesh_count, gpu_meshes, cpu_mesh_ids] : dirty_models) {
            gpu_meshes.reset((GPUMeshId*)::operator new(sizeof(GPUMeshId) * mesh_count));

            for (std::size_t m = 0; m < mesh_count; m++) {
                auto [transform, indices, vertex_count, positions, normals] =
                    *ecs.get<archetypes::Mesh, mesh_transform, model::MeshIndices, mesh_vertex_count_tag,
                             mesh_positions_tag, mesh_normals_tag>(cpu_mesh_ids[m]);

                uint32_t attribute_offset = 0;
                uint32_t stride = 0;
                GPUOffsets gpu_offsets{};
                gpu_offsets.start_offset = offset;
                if ((int)indices.stride != -1) {
                    gpu_offsets.attribute_mask |= GPUOffsets::Indices;
                    gpu_offsets.indices_offset = attribute_offset;
                    attribute_offset += sizeof(uint32_t) * indices.count;
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
                gpu_offsets.stride = stride;

                if (gpu_offsets.attribute_mask & GPUOffsets::Indices) {
                    auto* buf_view = buffer + offset + gpu_offsets.indices_offset;
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
                    auto* buf_view = buffer + offset + gpu_offsets.position_offset;
                    for (std::size_t i = 0; i < vertex_count; i++) {
                        *(glm::vec3*)buf_view = transform * glm::vec4{positions[i], 1.0f};

                        buf_view += gpu_offsets.stride;
                    }
                }
                if (gpu_offsets.attribute_mask & GPUOffsets::Normal) {
                    auto* buf_view = buffer + offset + gpu_offsets.normal_offset;
                    glm::mat3 normal_transform{glm::transpose(glm::inverse(transform))};
                    for (std::size_t i = 0; i < vertex_count; i++) {
                        *(glm::vec3*)buf_view = normal_transform * normals[i];

                        buf_view += gpu_offsets.stride;
                    }
                }

                offset += gpu_offsets.stride * vertex_count;

                std::println("start offset: {}", gpu_offsets.start_offset);
                std::println("stride: {}", gpu_offsets.stride);
                std::println("indices off: {}", gpu_offsets.indices_offset);
                std::println("position off: {}", gpu_offsets.position_offset);
                std::println("normal off: {}", gpu_offsets.normal_offset);
                std::println("attrib mask: {:b}", gpu_offsets.attribute_mask);
                if (gpu_offsets.attribute_mask & GPUOffsets::Indices) {
                    std::print("indices: ");
                    for (std::size_t i = 0; i < indices.count; i++) {
                        std::print(" {}",
                                   ((uint32_t*)(buffer + gpu_offsets.start_offset + gpu_offsets.indices_offset))[i]);
                    }
                    std::println("");
                }
                if (gpu_offsets.attribute_mask & GPUOffsets::Position) {
                    std::print("positions: ");
                    for (std::size_t i = 0; i < vertex_count; i++) {
                        auto pos = *(glm::vec3*)(buffer + gpu_offsets.start_offset + gpu_offsets.position_offset +
                                                 gpu_offsets.stride * i);
                        std::print(" [{}, {}, {}]", pos.x, pos.y, pos.z);
                    }
                    std::println("");
                }
                if (gpu_offsets.attribute_mask & GPUOffsets::Normal) {
                    std::print("normals: ");
                    for (std::size_t i = 0; i < vertex_count; i++) {
                        auto normal = *(glm::vec3*)(buffer + gpu_offsets.start_offset + gpu_offsets.normal_offset +
                                                    gpu_offsets.stride * i);
                        std::print(" [{}, {}, {}]", normal.x, normal.y, normal.z);
                    }
                    std::println("");
                }

                gpu_meshes[m] =
                    GPUMeshId{ecs.static_emplace_entity<archetypes::GPUMesh>(gpu_offsets, mesh_strorage_buf_id)};
            }
        }

        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);

        SDL_GPUTransferBufferLocation src{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };
        SDL_GPUBufferRegion dest{
            .buffer = gpu_buffer,
            .offset = 0,
            .size = total_buffer_size,
        };
        SDL_UploadToGPUBuffer(copy_pass, &src, &dest, false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
    }

    bool draw_model(ModelId model_id, SDL_GPUCommandBuffer* cmd_buf, SDL_GPURenderPass* render_pass,
                    uint32_t uniform_slot, uint32_t buffer_slot) {
        auto data = ecs.get<archetypes::Model, model_mesh_count_tag, model_meshes_tag, model_gpu_meshes_tag>(model_id);
        if (!data) return false;

        auto [mesh_count, cpu_meshes, gpu_meshes] = *data;

        ecs::Entity last_storage_buffer = ecs::null_id;
        SDL_GPUBuffer* gpu_buffer = nullptr;
        for (std::size_t m = 0; m < mesh_count; m++) {
            auto [indices] = *ecs.get<archetypes::Mesh, MeshIndices>(cpu_meshes[m]);
            auto [gpu_offsets, storage_buffer] = *ecs.get<archetypes::GPUMesh>(gpu_meshes[m]);

            if (storage_buffer != last_storage_buffer) {
                last_storage_buffer = storage_buffer;
                auto [buffer] = *ecs.get<archetypes::MeshStorage>(storage_buffer);
                gpu_buffer = buffer;
            }

            // SDL_BindGPUVertexStorageBuffers(render_pass, buffer_slot, &gpu_buffer, 1);
            // SDL_PushGPUVertexUniformData(cmd_buf, uniform_slot, &gpu_offsets, sizeof(GPUOffsets));
            // SDL_DrawGPUPrimitives(render_pass, (uint32_t)indices.count, 1, 0, 0);
            SDL_GPUBufferBinding index_binding{
                .buffer = gpu_buffer,
                .offset = gpu_offsets.start_offset + gpu_offsets.indices_offset,
            };
            SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_GPUBufferBinding attrib_buffer{
                .buffer = gpu_buffer,
                .offset = gpu_offsets.start_offset + gpu_offsets.position_offset,
            };
            SDL_BindGPUVertexBuffers(render_pass, 0, &attrib_buffer, 1);
            SDL_DrawGPUIndexedPrimitives(render_pass, (uint32_t)indices.count, 1, 0, 0, 0);
            break;
        }

        return true;
    }
}
