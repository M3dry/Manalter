#include "engine/imgui.hpp"
#include "imgui_.hpp"

#include "engine/engine.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

void check_vk_result(VkResult err) {
    if (err == 0) return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0) abort();
}

namespace engine::imgui {
    ImDrawData* draw_data = nullptr;

    bool state = false;

    bool enabled() {
        return state;
    }

    void init() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForVulkan(window);
        ImGui_ImplVulkan_InitInfo imgui_info{};
        imgui_info.UseDynamicRendering = true;
        imgui_info.Instance = instance;
        imgui_info.PhysicalDevice = physical_device;
        imgui_info.Device = device;
        imgui_info.QueueFamily = graphics_queue_family;
        imgui_info.Queue = graphics_queue;
        imgui_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
        imgui_info.Subpass = 0;
        imgui_info.MinImageCount = 2;
        imgui_info.ImageCount = 2;
        imgui_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        imgui_info.Allocator = nullptr;
        imgui_info.CheckVkResultFn = check_vk_result;
        imgui_info.PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain_format,
        };
        ImGui_ImplVulkan_Init(&imgui_info);
    }

    void destroy() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    bool polled_event(SDL_Event* ev) {
        if (!state) return false;

        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplSDL3_ProcessEvent(ev);

        bool is_key_event = ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP;
        bool is_mouse_event = ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev->type == SDL_EVENT_MOUSE_BUTTON_UP ||
                              ev->type == SDL_EVENT_MOUSE_WHEEL || ev->type == SDL_EVENT_MOUSE_MOTION;
        return (io.WantCaptureKeyboard && is_key_event) || (io.WantCaptureMouse && is_mouse_event);
    }

    void new_frame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void prepare_draw_data() {
        ImGui::Render();
        draw_data = ImGui::GetDrawData();
    }

    void render() {
        if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) return;

        ImGui_ImplVulkan_RenderDrawData(draw_data, get_cmd_buf());
    }
}
