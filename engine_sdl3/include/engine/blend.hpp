#pragma once

#include <SDL3/SDL_gpu.h>

namespace engine::blend {
    enum BlendFactor {
        InvalidFactor = SDL_GPU_BLENDFACTOR_INVALID,
        Zero = SDL_GPU_BLENDFACTOR_ZERO,
        One = SDL_GPU_BLENDFACTOR_ONE,
        SrcColor = SDL_GPU_BLENDFACTOR_SRC_COLOR,
        OneMinusSrcColor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
        DestColor = SDL_GPU_BLENDFACTOR_DST_COLOR,
        OneMinusDestColor = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR,
        SrcAlpha = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
        OneMinusSrcAlpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        DestAlpha = SDL_GPU_BLENDFACTOR_DST_ALPHA,
        OneMinusDestAlpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
        ConstantColor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR,
        OneMinusConstantColor = SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR,
        SrcAlphaSaturate = SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE,
    };

    enum BlendOp {
        InvalidOp = SDL_GPU_BLENDOP_INVALID,
        Add = SDL_GPU_BLENDOP_ADD,
        Subtract = SDL_GPU_BLENDOP_SUBTRACT,
        ReverseSubtract = SDL_GPU_BLENDOP_REVERSE_SUBTRACT,
        Min = SDL_GPU_BLENDOP_MIN,
        Max = SDL_GPU_BLENDOP_MAX,
    };

    enum ColorComponents : uint8_t {
        R = SDL_GPU_COLORCOMPONENT_R,
        G = SDL_GPU_COLORCOMPONENT_G,
        B = SDL_GPU_COLORCOMPONENT_B,
        A = SDL_GPU_COLORCOMPONENT_A,
    };

    class Make {
      public:
        constexpr Make() {
            _blend.enable_blend = false;
        };

        constexpr Make& src_color_blend_factor(BlendFactor blend_factor) {
            _blend.enable_blend = true;
            _blend.src_color_blendfactor = static_cast<SDL_GPUBlendFactor>(blend_factor);
            return *this;
        }

        constexpr Make& dest_color_blend_factor(BlendFactor blend_factor) {
            _blend.enable_blend = true;
            _blend.dst_color_blendfactor = static_cast<SDL_GPUBlendFactor>(blend_factor);
            return *this;
        }

        constexpr Make& src_alpha_blend_factor(BlendFactor blend_factor) {
            _blend.enable_blend = true;
            _blend.src_alpha_blendfactor = static_cast<SDL_GPUBlendFactor>(blend_factor);
            return *this;
        }

        constexpr Make& dest_alpha_blend_factor(BlendFactor blend_factor) {
            _blend.enable_blend = true;
            _blend.dst_alpha_blendfactor = static_cast<SDL_GPUBlendFactor>(blend_factor);
            return *this;
        }

        constexpr Make& color_blend(BlendOp op) {
            _blend.enable_blend = true;
            _blend.color_blend_op = static_cast<SDL_GPUBlendOp>(op);
            return *this;
        }

        constexpr Make& alpha_blend(BlendOp op) {
            _blend.enable_blend = true;
            _blend.alpha_blend_op = static_cast<SDL_GPUBlendOp>(op);
            return *this;
        }

        constexpr Make& color_write_mask(ColorComponents components) {
            _blend.color_write_mask = static_cast<uint8_t>(components);
            return *this;
        }

        constexpr Make& no_color_write_mask() {
            _blend.enable_color_write_mask = false;
            return *this;
        }

        constexpr operator SDL_GPUColorTargetBlendState() {
            return _blend;
        }

      private:
        SDL_GPUColorTargetBlendState _blend{};
    };

    inline constexpr SDL_GPUColorTargetBlendState basic = Make()
                                                               .src_color_blend_factor(SrcAlpha)
                                                               .dest_alpha_blend_factor(OneMinusSrcAlpha)
                                                               .color_blend(Add)
                                                               .src_alpha_blend_factor(SrcAlpha)
                                                               .dest_alpha_blend_factor(OneMinusSrcAlpha)
                                                               .alpha_blend(Add)
                                                               .no_color_write_mask();
}
