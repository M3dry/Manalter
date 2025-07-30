#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_timer.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include <SDL3/SDL.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <imgui.h>
#include <print>
#include <string>

#include "camera.hpp"
#include "input.hpp"

void abort(const std::string& msg) {
    std::println("FATAL ERROR: {}", msg);
    std::exit(-1);
}

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const std::span<uint8_t>& code, SDL_GPUShaderStage stage,
                           uint32_t samplers, uint32_t textures, uint32_t storage_buffers, uint32_t uniforms) {
    SDL_GPUShaderCreateInfo info{
        .code_size = code.size(),
        .code = code.data(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = samplers,
        .num_storage_textures = textures,
        .num_storage_buffers = storage_buffers,
        .num_uniform_buffers = uniforms,
        .props = 0,
    };

    return SDL_CreateGPUShader(device, &info);
}

SDL_GPUShader* load_shader_from_file(SDL_GPUDevice* device, const char* file, SDL_GPUShaderStage stage,
                                     uint32_t samplers, uint32_t textures, uint32_t storage_buffers,
                                     uint32_t uniforms) {
    std::size_t code_size;
    void* code = SDL_LoadFile(file, &code_size);
    SDL_GPUShader* shader =
        load_shader(device, {(uint8_t*)code, code_size}, stage, samplers, textures, storage_buffers, uniforms);
    if (shader == nullptr) {
        abort(std::format("load_shader_from-file: {}; file size: {}", SDL_GetError(), code_size));
    }
    SDL_free(code);

    return shader;
}

struct UniformBuffer {
    glm::mat4 mvp;
};

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

static Vertex vertices[]{
    {3.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 1.0f},  // top vertex
    {-3.0f, 0.0f, 3.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // bottom left vertex
    {0.0f, 0.0f, -3.0f, 1.0f, 0.0f, 1.0f, 1.0f}  // bottom right vertex
};

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        abort(std::format("SLD_INIT(): {}", SDL_GetError()));
    }

    // // Create SDL window graphics context
    SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_VULKAN;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_GPU example", 1920, 1200, window_flags);
    if (window == nullptr) {
        abort(std::format("SDL_CreateWindow(): {}", SDL_GetError()));
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    SDL_GPUDevice* gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (gpu_device == nullptr) {
        abort(std::format("SDL_CreateGPUDevice(): {}", SDL_GetError()));
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
        abort(std::format("SDL_ClaimWindowForGPUDevice(): {}", SDL_GetError()));
    }
    SDL_SetGPUSwapchainParameters(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    ImGui::StyleColorsDark();

    // Setup scaling
    // ImGuiStyle& style = ImGui::GetStyle();
    // style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    // style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);

    auto* ver_shader = load_shader_from_file(gpu_device, "./vertex.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 1);
    auto* frag_shader = load_shader_from_file(gpu_device, "fragment.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

    SDL_GPUBufferCreateInfo buffer_info{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(vertices),
        .props = 0,
    };
    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(gpu_device, &buffer_info);

    {
        SDL_GPUTransferBufferCreateInfo transfer_info{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(vertices),
            .props = 0,
        };
        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_info);
        Vertex* data = (Vertex*)SDL_MapGPUTransferBuffer(gpu_device, transfer_buffer, false);
        std::memcpy(data, vertices, sizeof(vertices));
        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buffer);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTransferBufferLocation location{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };

        SDL_GPUBufferRegion region{
            .buffer = vertex_buffer,
            .offset = 0,
            .size = sizeof(vertices),
        };
        SDL_UploadToGPUBuffer(copy_pass, &location, &region, true);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);

        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buffer);
    }

    SDL_GPUVertexBufferDescription vertex_buffer_desc{
        .slot = 0,
        .pitch = sizeof(Vertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };
    SDL_GPUVertexAttribute vertex_atrib[2]{{
                                               .location = 0,
                                               .buffer_slot = 0,
                                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                               .offset = 0,
                                           },
                                           {
                                               .location = 1,
                                               .buffer_slot = 0,
                                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                                               .offset = sizeof(float) * 3,
                                           }};
    SDL_GPUColorTargetDescription color_target_desc{
        .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
        .blend_state =
            SDL_GPUColorTargetBlendState{
                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                .enable_blend = true,
                .enable_color_write_mask = false,
            },
    };
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info{
        .vertex_shader = ver_shader,
        .fragment_shader = frag_shader,
        .vertex_input_state =
            SDL_GPUVertexInputState{
                .vertex_buffer_descriptions = &vertex_buffer_desc,
                .num_vertex_buffers = 1,
                .vertex_attributes = vertex_atrib,
                .num_vertex_attributes = 2,
            },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info =
            SDL_GPUGraphicsPipelineTargetInfo{
                .color_target_descriptions = &color_target_desc,
                .num_color_targets = 1,
                .has_depth_stencil_target = false,
            },
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_info);

    glm::vec3 cam_pos = {0.0f, 10.0f, 10.0f};
    Camera cam{cam_pos, glm::vec3(0.0f), glm::vec3{0.0f, -1.0f, 0.0f}, 90.0f, 16.0f / 10.0f};
    InputHandler ih{};


    uint64_t accum = 0;
    uint64_t last_time = SDL_GetTicks();

    bool done = false;
    while (!done) {
        uint64_t time = SDL_GetTicks();
        uint64_t frame_time = time - last_time;
        last_time = time;
        accum += frame_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (!io.WantCaptureKeyboard && (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)) {
                ih.update(event.key);
            }
        }

        while (accum >= 1000/60) {
            accum -= 1000/60;

            if (ih.is_held(SDL_SCANCODE_W)) {
                cam_pos.z -= 1.0f;
            }
            if (ih.is_held(SDL_SCANCODE_S)) {
                cam_pos.z += 1.0f;
            }
            if (ih.is_held(SDL_SCANCODE_A)) {
                cam_pos.x -= 1.0f;
            }
            if (ih.is_held(SDL_SCANCODE_D)) {
                cam_pos.x += 1.0f;
            }

            cam.set_position(cam_pos);
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        SDL_GPUTexture* swapchain_texture;
        SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr);

        if (swapchain_texture == nullptr) {
            SDL_SubmitGPUCommandBuffer(command_buffer);
            continue;
        }

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        {
            ImGui::Begin("Camera");

            ImGui::InputFloat3("Position", glm::value_ptr(cam_pos));

            ImGui::End();
        }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        target_info.clear_color = SDL_FColor{0.f, 0.f, 0.f, 1.f};
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
        SDL_GPUBufferBinding buffer_binding{
            .buffer = vertex_buffer,
            .offset = 0,
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);

        UniformBuffer uniform_buffer{cam.get_view_projection()};
        SDL_PushGPUVertexUniformData(command_buffer, 0, &uniform_buffer, sizeof(UniformBuffer));

        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
        if (!is_minimized) ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_WaitForGPUIdle(gpu_device);

    SDL_ReleaseGPUShader(gpu_device, ver_shader);
    SDL_ReleaseGPUShader(gpu_device, frag_shader);
    SDL_ReleaseGPUBuffer(gpu_device, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
    SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
