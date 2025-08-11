#pragma once

#include "engine/gpu.hpp"
#include "typeset.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float1.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <vector>

namespace engine::pipeline {
    template <typename T> struct to_vertex_elem;
    template <typename T>
        requires(typeset::in_set_v<T, glm::vec1, float>)
    struct to_vertex_elem<T> {
        static constexpr SDL_GPUVertexElementFormat format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
        static constexpr uint32_t offset = sizeof(float);
    };

    template <> struct to_vertex_elem<glm::vec2> {
        static constexpr SDL_GPUVertexElementFormat format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        static constexpr uint32_t offset = 2 * sizeof(float);
    };

    template <> struct to_vertex_elem<glm::vec3> {
        static constexpr SDL_GPUVertexElementFormat format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        static constexpr uint32_t offset = 3 * sizeof(float);
    };

    template <> struct to_vertex_elem<glm::quat> {
        static constexpr SDL_GPUVertexElementFormat format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        static constexpr uint32_t offset = 4 * sizeof(float);
    };

    template <typename T>
    concept HasFormat = requires() {
        { to_vertex_elem<T>::format } -> std::same_as<const SDL_GPUVertexElementFormat&>;
    };

    template <typename T>
    concept HasSize = requires() {
        { to_vertex_elem<T>::offset } -> std::same_as<const uint32_t&>;
    };

    template <typename T>
    concept IsVertexAttrib = HasSize<T> && HasFormat<T>;

    template <HasSize E> struct blank {};
    template <HasSize E> struct to_vertex_elem<blank<E>> {
        static constexpr uint32_t offset = to_vertex_elem<E>::offset;
    };

    template <HasSize... Ts> class VertexAttributes {
      private:
        static constexpr uint32_t vertex_elements = (0 + ... + HasFormat<Ts>);

      public:
        uint32_t buffer_slot = 0;
        uint32_t start_location = 0;

        operator std::array<SDL_GPUVertexAttribute, vertex_elements>() {
            return to_arr();
        }

        operator std::vector<SDL_GPUVertexAttribute>() {
            return to_vec();
        }

        std::array<SDL_GPUVertexAttribute, vertex_elements> to_arr() {
            std::array<SDL_GPUVertexAttribute, vertex_elements> out;

            write_into(out.data());
            return out;
        }

        std::vector<SDL_GPUVertexAttribute> to_vec() {
            std::vector<SDL_GPUVertexAttribute> out;
            out.resize(vertex_elements);

            write_into(out.data());
            return out;
        }

        void write_into(SDL_GPUVertexAttribute* arr) {
            uint32_t offset_accum = 0;
            uint32_t location_accum = start_location;

            uint32_t i = 0;
            auto assign = [&]<typename T>(uint32_t ix) {
                if constexpr (HasFormat<T>) {
                    arr[ix] = SDL_GPUVertexAttribute{
                        .location = location_accum++,
                        .buffer_slot = buffer_slot,
                        .format = to_vertex_elem<T>::format,
                        .offset = offset_accum,
                    };
                }
                offset_accum += to_vertex_elem<T>::offset;
            };

            (assign.template operator ()<Ts>(i++), ...);
        }
    };

    template <HasSize... Ts> struct VertexBuffer {
        uint32_t slot = 0;
        bool instance = false;

        operator SDL_GPUVertexBufferDescription() {
            return SDL_GPUVertexBufferDescription{
                .slot = slot,
                .pitch = (0 + ... + to_vertex_elem<Ts>::offset),
                .input_rate = instance ? SDL_GPU_VERTEXINPUTRATE_INSTANCE : SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0,
            };
        }
    };

    enum PrimitiveType {
        TriangleList = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        TriangleStrip = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
        LineList = SDL_GPU_PRIMITIVETYPE_LINELIST,
        LineStrip = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
        PointList = SDL_GPU_PRIMITIVETYPE_POINTLIST,
    };

    enum FillMode {
        Fill = SDL_GPU_FILLMODE_FILL,
        Line = SDL_GPU_FILLMODE_LINE,
    };

    enum CullMode {
        CullNone = SDL_GPU_CULLMODE_NONE,
        CullFront = SDL_GPU_CULLMODE_FRONT,
        CullBack = SDL_GPU_CULLMODE_BACK,
    };

    enum FrontFace {
        CCW = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        CW = SDL_GPU_FRONTFACE_CLOCKWISE,
    };

    enum SampleCount {
        OneSample = SDL_GPU_SAMPLECOUNT_1,
        TwoSamples = SDL_GPU_SAMPLECOUNT_2,
        FourSamples = SDL_GPU_SAMPLECOUNT_4,
        EightSamples = SDL_GPU_SAMPLECOUNT_8,
    };

    enum CompareOp {
        Invalid = SDL_GPU_COMPAREOP_INVALID,
        Never = SDL_GPU_COMPAREOP_NEVER,
        Less = SDL_GPU_COMPAREOP_LESS,
        Equal = SDL_GPU_COMPAREOP_EQUAL,
        LessOrEqual = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
        Greater = SDL_GPU_COMPAREOP_GREATER,
        NotEqual = SDL_GPU_COMPAREOP_NOT_EQUAL,
        GreaterOrEqual = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL,
        Always = SDL_GPU_COMPAREOP_ALWAYS
    };

    class ColorTarget {
      public:
        constexpr ColorTarget() {
            target.blend_state.enable_blend = false;
        };

        constexpr ColorTarget& format(SDL_GPUTextureFormat format) {
            target.format = format;
            return *this;
        }

        constexpr ColorTarget& blend_state(SDL_GPUColorTargetBlendState blend_state) {
            target.blend_state = blend_state;
            return *this;
        }

        constexpr operator SDL_GPUColorTargetDescription() {
            return target;
        }

      private:
        SDL_GPUColorTargetDescription target{};
    };

    class Make {
      public:
        constexpr Make()
            : sdl_pipeline({
                  .vertex_shader = nullptr,
                  .fragment_shader = nullptr,
                  .vertex_input_state =
                      SDL_GPUVertexInputState{
                          .vertex_buffer_descriptions = nullptr,
                          .num_vertex_buffers = 0,
                          .vertex_attributes = nullptr,
                          .num_vertex_attributes = 0,
                      },
                  .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                  .rasterizer_state =
                      SDL_GPURasterizerState{
                          .fill_mode = SDL_GPU_FILLMODE_FILL,
                          .cull_mode = SDL_GPU_CULLMODE_NONE,
                          .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
                          .enable_depth_bias = false,
                          .enable_depth_clip = false,
                      },
                  .multisample_state =
                      SDL_GPUMultisampleState{
                          .sample_count = SDL_GPU_SAMPLECOUNT_1,
                          .sample_mask = 0,
                          .enable_mask = false,
                      },
                  .depth_stencil_state =
                      SDL_GPUDepthStencilState{
                          .compare_op = SDL_GPU_COMPAREOP_INVALID,
                          .write_mask = 0xFF,
                          .enable_depth_test = false,
                          .enable_depth_write = false,
                          .enable_stencil_test = false,
                      },
                  .target_info =
                      SDL_GPUGraphicsPipelineTargetInfo{
                          .color_target_descriptions = nullptr,
                          .num_color_targets = 0,
                          .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                          .has_depth_stencil_target = false,
                      },
                  .props = 0,
              }) {};

        constexpr Make& vertex_shader(SDL_GPUShader* shader) {
            sdl_pipeline.vertex_shader = shader;
            return *this;
        }

        constexpr Make& fragment_shader(SDL_GPUShader* shader) {
            sdl_pipeline.fragment_shader = shader;
            return *this;
        }

        constexpr Make& shaders(SDL_GPUShader* vertex, SDL_GPUShader* fragment) {
            sdl_pipeline.vertex_shader = vertex;
            sdl_pipeline.fragment_shader = fragment;
            return *this;
        }

        constexpr Make& primitive(PrimitiveType draw_type) {
            sdl_pipeline.primitive_type = static_cast<SDL_GPUPrimitiveType>(draw_type);
            return *this;
        }

        constexpr Make& vertex_buffers(std::vector<SDL_GPUVertexBufferDescription> vertex_buffers) {
            _vertex_buffers = vertex_buffers;

            sdl_pipeline.vertex_input_state.vertex_buffer_descriptions = _vertex_buffers.data();
            sdl_pipeline.vertex_input_state.num_vertex_buffers = static_cast<uint32_t>(_vertex_buffers.size());

            return *this;
        }

        constexpr Make& vertex_attributes(std::vector<SDL_GPUVertexAttribute> vertex_attributes) {
            _vertex_attributes = vertex_attributes;

            sdl_pipeline.vertex_input_state.vertex_attributes = _vertex_attributes.data();
            sdl_pipeline.vertex_input_state.num_vertex_attributes = static_cast<uint32_t>(_vertex_attributes.size());

            return *this;
        }

        constexpr Make& fill_mode(FillMode fill_mode) {
            sdl_pipeline.rasterizer_state.fill_mode = static_cast<SDL_GPUFillMode>(fill_mode);
            return *this;
        }

        constexpr Make& cull_mode(CullMode cull_mode) {
            sdl_pipeline.rasterizer_state.cull_mode = static_cast<SDL_GPUCullMode>(cull_mode);
            return *this;
        }

        constexpr Make& front_face(FrontFace front_face) {
            sdl_pipeline.rasterizer_state.front_face = static_cast<SDL_GPUFrontFace>(front_face);
            return *this;
        }

        constexpr Make& depth_bias(float constant_factor, float slope_factor, float clamp) {
            sdl_pipeline.rasterizer_state.enable_depth_bias = true;
            sdl_pipeline.rasterizer_state.depth_bias_constant_factor = constant_factor;
            sdl_pipeline.rasterizer_state.depth_bias_clamp = clamp;
            sdl_pipeline.rasterizer_state.depth_bias_slope_factor = slope_factor;

            return *this;
        }

        constexpr Make& no_depth_bias() {
            sdl_pipeline.rasterizer_state.enable_depth_bias = false;
            return *this;
        }

        constexpr Make& depth_clip() {
            sdl_pipeline.rasterizer_state.enable_depth_clip = true;
            return *this;
        }

        constexpr Make& no_depth_clip() {
            sdl_pipeline.rasterizer_state.enable_depth_clip = false;
            return *this;
        }

        constexpr Make& sample_count(SampleCount samples) {
            sdl_pipeline.multisample_state.sample_count = static_cast<SDL_GPUSampleCount>(samples);
            return *this;
        }

        constexpr Make& color_targets(std::vector<SDL_GPUColorTargetDescription> color_target_descs) {
            _color_targets = color_target_descs;

            sdl_pipeline.target_info.color_target_descriptions = _color_targets.data();
            sdl_pipeline.target_info.num_color_targets = static_cast<uint32_t>(_color_targets.size());

            return *this;
        }

        constexpr Make& depth_stencil_format(SDL_GPUTextureFormat format) {
            sdl_pipeline.target_info.has_depth_stencil_target = true;
            sdl_pipeline.target_info.depth_stencil_format = format;
            return *this;
        }

        constexpr Make& no_depth_stencil() {
            sdl_pipeline.target_info.has_depth_stencil_target = false;
            return *this;
        }

        constexpr Make& depth_testing(CompareOp compare_op) {
            sdl_pipeline.depth_stencil_state.enable_depth_test = true;
            sdl_pipeline.depth_stencil_state.compare_op = static_cast<SDL_GPUCompareOp>(compare_op);
            return *this;
        }

        constexpr Make& no_depth_testing() {
            sdl_pipeline.depth_stencil_state.enable_depth_test = false;
            return *this;
        }

        constexpr Make& depth_write() {
            sdl_pipeline.depth_stencil_state.enable_depth_write = true;
            return *this;
        }

        constexpr Make& no_depth_write() {
            sdl_pipeline.depth_stencil_state.enable_depth_write = false;
            return *this;
        }

        struct StencilTest {
            SDL_GPUStencilOpState back_stencil_state;
            SDL_GPUStencilOpState front_stencil_state;
            uint8_t compare_mask;
            uint8_t write_mask;
        };

        constexpr Make& stencil_test(StencilTest stencil_test) {
            sdl_pipeline.depth_stencil_state.enable_stencil_test = true;
            sdl_pipeline.depth_stencil_state.back_stencil_state = stencil_test.back_stencil_state;
            sdl_pipeline.depth_stencil_state.front_stencil_state = stencil_test.front_stencil_state;
            sdl_pipeline.depth_stencil_state.compare_mask = stencil_test.compare_mask;
            sdl_pipeline.depth_stencil_state.write_mask = stencil_test.write_mask;
            return *this;
        }

        constexpr Make& no_stencil_test() {
            sdl_pipeline.depth_stencil_state.enable_stencil_test = false;
            return *this;
        }

        SDL_GPUGraphicsPipeline* create() {
            return SDL_CreateGPUGraphicsPipeline(engine::gpu_device, &sdl_pipeline);
        }

      private:
        SDL_GPUGraphicsPipelineCreateInfo sdl_pipeline;
        std::vector<SDL_GPUVertexBufferDescription> _vertex_buffers{};
        std::vector<SDL_GPUVertexAttribute> _vertex_attributes{};
        std::vector<SDL_GPUColorTargetDescription> _color_targets{};
    };
}
