#include "engine/imgui.hpp"
#include "imgui_c.hpp"

#include "engine/gpu.hpp"
#include "engine/window.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

bool imgui_state = true;
ImDrawData* draw_data = nullptr;

namespace engine::imgui {
    bool enabled() {
        return imgui_state;
    }

    void enable(bool state) {
        imgui_state = state;
    }

    void init() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForSDLGPU(win);
        ImGui_ImplSDLGPU3_InitInfo init_info = {};
        init_info.Device = gpu_device;
        init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, win);
        init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        ImGui_ImplSDLGPU3_Init(&init_info);
    }

    void deinit() {
        SDL_WaitForGPUIdle(gpu_device);
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();
    }

    void new_frame() {
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void prepare_data(SDL_GPUCommandBuffer* cmd_buf) {
        ImGui::Render();
        draw_data = ImGui::GetDrawData();
        Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);
    }

    void draw(SDL_GPUCommandBuffer* cmd_buf, SDL_GPURenderPass* render_pass) {
        if (draw_data != nullptr)
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, render_pass);
    }
}
