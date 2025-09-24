#include "gpu_c.hpp"
#include "imgui_c.hpp"
#include "texture_c.hpp"
#include "window_c.hpp"
#include <engine/engine.hpp>
#include <engine/gpu.hpp>
#include <engine/window.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>

#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <print>

namespace engine {
    void init_sdl(const InitConfig& init_conf) {
        window::init(init_conf.window_name);
        gpu::init();
    }

    void init(const InitConfig& init_conf) {
        init_sdl(init_conf);
        imgui::init();
        texture::init();
    }

    void deinit() {
        texture::deinit();
        imgui::deinit();
        gpu::deinit();
        window::deinit();

        SDL_Quit();
    }
}
