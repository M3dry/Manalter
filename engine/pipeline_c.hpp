#pragma once

#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include <type_traits>

#include <cstring>

#include "engine/pipeline.hpp"
#include "multiarray.hpp"

template <typename T, std::size_t N> class SmallBuffer {
  public:
    static constexpr std::size_t ActualInline = std::max(N * sizeof(T), sizeof(T*));

    uint8_t inline_storage[ActualInline];

    SmallBuffer(std::size_t size)
        requires std::default_initializable<T>
    {
        T* ptr;
        if (is_inline(size)) {
            ptr = (T*)inline_storage;
        } else {
            ptr = (T*)::operator new(sizeof(T) * size);
            std::memcpy(&inline_storage, &ptr, sizeof(T*));
        }

        for (std::size_t i = 0; i < size; i++) {
            std::construct_at<T>(ptr + i);
        }
    }
    template <typename U>
        requires std::constructible_from<T, U>
    SmallBuffer(std::span<U> buffer) {
        T* ptr;
        if (is_inline(buffer.size())) {
            ptr = (T*)inline_storage;
        } else {
            ptr = (T*)::operator new(sizeof(T) * buffer.size());
            std::memcpy(&inline_storage, &ptr, sizeof(T*));
        }

        for (std::size_t i = 0; i < buffer.size(); i++) {
            std::construct_at(ptr + i, buffer[i]);
        } }
    SmallBuffer(const SmallBuffer& buf) {
        std::memcpy(&inline_storage, &buf.inline_storage, ActualInline);
    }
    SmallBuffer(SmallBuffer&& buf) noexcept {
        std::memcpy(&inline_storage, &buf.inline_storage, ActualInline);
        std::memset(&buf.inline_storage, 0, ActualInline);
    }

    bool equals_memcmp(const SmallBuffer& other, std::size_t size) {
        if (is_inline(size)) {
            return std::memcmp(&inline_storage, &other.inline_storage, size * sizeof(T)) == 0;
        }

        T* ptr;
        T* other_ptr;
        std::memcpy(&ptr, &inline_storage, sizeof(T*));
        std::memcpy(&other_ptr, &other.inline_storage, sizeof(T*));

        return std::memcmp(ptr, other_ptr, size * sizeof(T)) == 0;
    }

    static constexpr bool is_inline(std::size_t size) {
        return size <= ActualInline;
    }

    const T* data(std::size_t size) const {
        if (is_inline(size)) {
            return (T*)inline_storage;
        } else {
            T* ptr;
            std::memcpy(&ptr, &inline_storage, sizeof(T*));
            return ptr;
        }
    }

    T* data(std::size_t size) {
        if (is_inline(size)) {
            return (T*)inline_storage;
        } else {
            T* ptr;
            std::memcpy(&ptr, &inline_storage, sizeof(T*));
            return ptr;
        }
    }

    void destroy(std::size_t size) {
        T* ptr;
        if (is_inline(size)) {
            ptr = (T*)inline_storage;
        } else {
            std::memcpy(&ptr, &inline_storage, sizeof(T*));
        }

        for (std::size_t i = 0; i < size; i++) {
            std::destroy_at<T>(ptr + i);
        }

        if (!is_inline(size)) ::operator delete(ptr);

        size = 0;
    }
};

namespace engine::pipeline {
    struct PipelineDesc {
        struct VertexBuffer {
            uint8_t slot : 7;
            uint8_t input_rate : 1;
            uint8_t pitch[2];

            VertexBuffer(SDL_GPUVertexBufferDescription desc) {
                slot = (uint8_t)desc.slot;
                input_rate = desc.input_rate;

                auto pitch_u16 = (uint16_t)desc.pitch;
                std::memcpy(pitch, &pitch_u16, sizeof(uint16_t));
            }
        };

        struct VertexAttribute {
            uint8_t offset[3];
            uint8_t buffer_slot : 5;
            uint8_t location : 6;
            uint8_t element_format : 5;

            VertexAttribute(SDL_GPUVertexAttribute desc) {
                offset[0] = (uint8_t)(desc.offset & 0xFF);
                offset[1] = (uint8_t)((desc.offset >> 8) & 0xFF);
                offset[2] = (uint8_t)((desc.offset >> 16) & 0xFF);
                buffer_slot = (uint8_t)desc.buffer_slot;
                location = (uint8_t)desc.location;
                element_format = desc.format;
            }
        };

        struct ColorTarget {
            SDL_GPUTextureFormat format : 7;
            uint8_t enable_blend : 1;
            SDL_GPUBlendFactor src_color_blend_factor : 4;
            SDL_GPUBlendFactor dst_color_blend_factor : 4;
            SDL_GPUBlendFactor src_alpha_blend_factor : 4;
            SDL_GPUBlendFactor dst_alpha_blend_factor : 4;
            SDL_GPUBlendOp color_blend_op : 3;
            SDL_GPUBlendOp alpha_blend_op : 3;
            uint8_t enable_color_write_mask : 1;

            ColorTarget(SDL_GPUColorTargetDescription desc) {
                format = desc.format;
                enable_blend = desc.blend_state.enable_blend;
                src_color_blend_factor = desc.blend_state.src_color_blendfactor;
                dst_color_blend_factor = desc.blend_state.dst_color_blendfactor;
                src_alpha_blend_factor = desc.blend_state.src_alpha_blendfactor;
                dst_alpha_blend_factor = desc.blend_state.dst_alpha_blendfactor;
                color_blend_op = desc.blend_state.color_blend_op;
                alpha_blend_op = desc.blend_state.alpha_blend_op;
                enable_color_write_mask = desc.blend_state.color_write_mask;
            }
        };

        static constexpr auto memcmp_offset = 35;

        SmallBuffer<VertexBuffer, 3> vertex_buffer;
        SmallBuffer<VertexAttribute, 3> vertex_attributes;
        SmallBuffer<ColorTarget, 2> color_target_descriptions;
        SDL_GPUTextureFormat depth_stencil_format : 7;
        SDL_GPUFrontFace front_face : 1;
        uint16_t vertex_shader_id;
        uint16_t fragment_shader_id;
        uint8_t depth_compare_op : 3;
        SDL_GPUPrimitiveType primitive_type : 3;
        SDL_GPUCullMode cull_mode : 2;
        uint8_t vertex_attribute_count : 6;
        SDL_GPUSampleCount sample_count : 2;
        uint8_t color_target_count : 5;
        uint8_t depth_bias : 1;
        uint8_t depth_clip : 1;
        uint8_t depth_test : 1;
        uint8_t compare_mask;
        uint8_t write_mask;
        uint32_t stencil_states : 24;
        float depth_bias_constant_factor;
        float depth_bias_clamp;
        float depth_bias_slope_factor;
        uint8_t vertex_buffer_count : 5;
        SDL_GPUFillMode fill_mode : 1;
        uint8_t depth_write : 1;
        uint8_t stencil_test : 1;
        uint8_t has_depth_stencil_target : 1;
        uint32_t padding : 23 = 0;

        PipelineDesc(const Make& pipeline)
            : vertex_buffer(std::span{pipeline._vertex_buffers.get(), pipeline._sdl_pipeline.vertex_input_state.num_vertex_buffers}),
              vertex_attributes(std::span{pipeline._vertex_attributes.get(), pipeline._sdl_pipeline.vertex_input_state.num_vertex_attributes}),
              color_target_descriptions(std::span{pipeline._color_targets.get(), pipeline._sdl_pipeline.target_info.num_color_targets}) {
            assert(pipeline._sdl_pipeline.vertex_input_state.num_vertex_buffers <= 31);
            vertex_buffer_count = (uint8_t)pipeline._sdl_pipeline.vertex_input_state.num_vertex_buffers;
            assert(pipeline._sdl_pipeline.vertex_input_state.num_vertex_attributes <= 63);
            vertex_attribute_count = (uint8_t)pipeline._sdl_pipeline.vertex_input_state.num_vertex_attributes;
            assert(pipeline._sdl_pipeline.target_info.num_color_targets <= 31);
            color_target_count = (uint8_t)pipeline._sdl_pipeline.target_info.num_color_targets;

            auto sdl_pipeline = pipeline._sdl_pipeline;
            depth_stencil_format = sdl_pipeline.target_info.depth_stencil_format;
            front_face = sdl_pipeline.rasterizer_state.front_face;
            vertex_shader_id = pipeline._vertex_shader_id;
            fragment_shader_id = pipeline._fragment_shader_id;
            depth_compare_op = sdl_pipeline.depth_stencil_state.compare_op - 1; // ignore the INVALID enum
            primitive_type = sdl_pipeline.primitive_type;
            cull_mode = sdl_pipeline.rasterizer_state.cull_mode;
            sample_count = sdl_pipeline.multisample_state.sample_count;
            depth_bias = sdl_pipeline.rasterizer_state.enable_depth_bias;
            depth_clip = sdl_pipeline.rasterizer_state.enable_depth_clip;
            depth_test = sdl_pipeline.depth_stencil_state.enable_depth_test;
            compare_mask = sdl_pipeline.depth_stencil_state.compare_mask;
            write_mask = sdl_pipeline.depth_stencil_state.write_mask;
            set_front_stencil_state(sdl_pipeline.depth_stencil_state.front_stencil_state);
            set_back_stencil_state(sdl_pipeline.depth_stencil_state.back_stencil_state);
            depth_bias_constant_factor = sdl_pipeline.rasterizer_state.depth_bias_constant_factor;
            depth_bias_clamp = sdl_pipeline.rasterizer_state.depth_bias_clamp;
            depth_bias_slope_factor = sdl_pipeline.rasterizer_state.depth_bias_slope_factor;
            fill_mode = sdl_pipeline.rasterizer_state.fill_mode;
            depth_write = sdl_pipeline.depth_stencil_state.enable_depth_write;
            stencil_test = sdl_pipeline.depth_stencil_state.enable_stencil_test;
            has_depth_stencil_target = sdl_pipeline.target_info.has_depth_stencil_target;
        }
        PipelineDesc(PipelineDesc&& p) noexcept
            : vertex_buffer(std::move(p.vertex_buffer)), vertex_attributes(std::move(p.vertex_attributes)),
              color_target_descriptions(std::move(p.color_target_descriptions)) {
            vertex_buffer_count = p.vertex_buffer_count;
            vertex_attribute_count = p.vertex_attribute_count;
            color_target_count = p.color_target_count;
            depth_stencil_format = p.depth_stencil_format;
            front_face = p.front_face;
            vertex_shader_id = p.vertex_shader_id;
            fragment_shader_id = p.fragment_shader_id;
            depth_compare_op = p.depth_compare_op;
            primitive_type = p.primitive_type;
            cull_mode = p.cull_mode;
            sample_count = p.sample_count;
            depth_bias = p.depth_bias;
            depth_clip = p.depth_clip;
            depth_test = p.depth_test;
            compare_mask = p.compare_mask;
            write_mask = p.write_mask;
            stencil_states = p.stencil_states;
            depth_bias_constant_factor = p.depth_bias_constant_factor;
            depth_bias_clamp = p.depth_bias_clamp;
            depth_bias_slope_factor = p.depth_bias_slope_factor;
            fill_mode = p.fill_mode;
            depth_write = p.depth_write;
            stencil_test = p.stencil_test;
            has_depth_stencil_target = p.has_depth_stencil_target;

            p.vertex_buffer_count = 0;
            p.vertex_attribute_count = 0;
            p.color_target_count = 0;
        };

        ~PipelineDesc() {
            vertex_buffer.destroy(vertex_buffer_count);
            vertex_attributes.destroy(vertex_attribute_count);
            color_target_descriptions.destroy(color_target_count);
        }

        bool operator==(const PipelineDesc& other) {
            if (std::memcmp((uint8_t*)this + memcmp_offset, (uint8_t*)&other + memcmp_offset,
                            sizeof(PipelineDesc) - memcmp_offset) != 0)
                return false;

            if (vertex_buffer_count != other.vertex_buffer_count ||
                vertex_attribute_count != other.vertex_attribute_count ||
                color_target_count != other.color_target_count)
                return false;

            if (!vertex_buffer.equals_memcmp(other.vertex_buffer, vertex_buffer_count) ||
                !vertex_attributes.equals_memcmp(other.vertex_attributes, vertex_attribute_count) ||
                !color_target_descriptions.equals_memcmp(other.color_target_descriptions, color_target_count))
                return false;

            return true;
        }

        void set_front_stencil_state(SDL_GPUStencilOpState state) {
            set_stencil_state(state, 0);
        }

        void set_back_stencil_state(SDL_GPUStencilOpState state) {
            set_stencil_state(state, 12);
        }

      private:
        void set_stencil_state(SDL_GPUStencilOpState state, uint32_t initial_offset) {
            static constexpr auto stencil_op = [](SDL_GPUStencilOp op) -> uint8_t { return op - 1; };
            static constexpr auto compare_op = [](SDL_GPUCompareOp op) -> uint8_t { return op - 1; };

            static constexpr uint32_t element_mask = 0x7;
            uint32_t packed = ((stencil_op(state.fail_op) & element_mask) << initial_offset) |
                              ((stencil_op(state.pass_op) & element_mask) << (initial_offset + 3)) |
                              ((stencil_op(state.depth_fail_op) & element_mask) << (initial_offset + 6)) |
                              ((compare_op(state.compare_op) & element_mask) << (initial_offset + 9));
            uint32_t mask = ~(0xFFFu << initial_offset);
            stencil_states = (stencil_states & mask) | packed;
        }
    };

    static_assert(sizeof(PipelineDesc) <= 64);

    extern multi_vector<PipelineDesc, SDL_GPUGraphicsPipeline*> pipelines;

    PipelineId create_pipeline(const Make& desc);
    std::optional<PipelineId> pipeline_exists(const PipelineDesc& desc);
}
