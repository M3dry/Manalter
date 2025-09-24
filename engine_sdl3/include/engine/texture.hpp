#pragma once

#include "engine/util.hpp"

#include <SDL3/SDL_gpu.h>

namespace engine::texture {
    MAKE_WRAPPED_ID(TextureId);

    // 1x1 white texture
    extern SDL_GPUTexture* fallback;
}
