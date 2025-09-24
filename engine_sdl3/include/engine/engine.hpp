#pragma once

#include <SDL3/SDL_gpu.h>

namespace engine {
    struct InitConfig {
        const char* window_name;
    };

    void init(const InitConfig& init_conf);
    void deinit();
}
