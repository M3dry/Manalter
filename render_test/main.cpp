#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_timer.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <glm/matrix.hpp>
#include <imgui.h>
#include <print>
#include <string>

#include "camera.hpp"
#include "input.hpp"
#include "mesh.hpp"

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

struct CubeUniformBuffer {
    glm::mat4 mvp;
    glm::vec4 color;
};

struct Vertex {
    float x, y, z;
    float r, g, b, a;
    float u, v;
};

static Vertex vertices[]{
    {3.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},  // tip
    {-3.0f, 0.0f, 3.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f}, // right (if tip is bottom)
    {0.0f, 0.0f, -3.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f, 1.0f}  // left (if tip is bottom)
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);

    auto* ver_shader = load_shader_from_file(gpu_device, "./vertex.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 1);
    auto* frag_shader = load_shader_from_file(gpu_device, "./fragment.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);

    SDL_GPUVertexBufferDescription vertex_buffer_desc{
        .slot = 0,
        .pitch = sizeof(Vertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };
    SDL_GPUVertexAttribute vertex_atrib[3]{{
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
                                           },
                                           {
                                               .location = 2,
                                               .buffer_slot = 0,
                                               .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                               .offset = sizeof(float) * 7,
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
                .num_vertex_attributes = 3,
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

    auto* cube_ver_shader =
        load_shader_from_file(gpu_device, "./cube_vertex.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 1);
    auto* cube_frag_shader =
        load_shader_from_file(gpu_device, "./cube_fragment.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

    SDL_GPUVertexBufferDescription cube_vertex_buffer_desc{
        .slot = 0,
        .pitch = sizeof(glm::vec3),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };
    SDL_GPUVertexAttribute cube_vertex_atrib{
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = 0,
    };
    SDL_GPUGraphicsPipelineCreateInfo cube_pipeline_info{
        .vertex_shader = cube_ver_shader,
        .fragment_shader = cube_frag_shader,
        .vertex_input_state =
            SDL_GPUVertexInputState{
                .vertex_buffer_descriptions = &cube_vertex_buffer_desc,
                .num_vertex_buffers = 1,
                .vertex_attributes = &cube_vertex_atrib,
                .num_vertex_attributes = 1,
            },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info =
            SDL_GPUGraphicsPipelineTargetInfo{
                .color_target_descriptions = &color_target_desc,
                .num_color_targets = 1,
                .has_depth_stencil_target = false,
            },
    };
    SDL_GPUGraphicsPipeline* cube_pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &cube_pipeline_info);

    SDL_GPUBufferCreateInfo buffer_info{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(vertices),
        .props = 0,
    };
    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(gpu_device, &buffer_info);

    glm::vec3 cube_vertices[] = {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec3{0.5f, -0.5f, -0.5f},
                                 glm::vec3{0.5f, 0.5f, -0.5f},   glm::vec3{-0.5f, 0.5f, -0.5f},
                                 glm::vec3{-0.5f, -0.5f, 0.5f},  glm::vec3{0.5f, -0.5f, 0.5f},
                                 glm::vec3{0.5f, 0.5f, 0.5f},    glm::vec3{-0.5f, 0.5f, 0.5f}};
    uint32_t cube_indexes[] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 7, 3, 0, 0, 4, 7,
                               6, 2, 1, 1, 5, 6, 0, 1, 5, 5, 4, 0, 3, 2, 6, 6, 7, 3};
    Mesh cube{
        .indexes = cube_indexes,
        .index_count = 36,
        .vertices = cube_vertices,
        .vertex_count = 8,
    };
    GPUMesh gpu_cube;
    glm::vec4 cube_color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::mat4 cube_model_mat = glm::mat4{1.0f};

    SDL_GPUSamplerCreateInfo sampler_info{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    };
    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(gpu_device, &sampler_info);
    SDL_Surface* texture_surface = IMG_Load("./fernando.png");

    SDL_GPUTextureCreateInfo texture_info{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<uint32_t>(texture_surface->w),
        .height = static_cast<uint32_t>(texture_surface->h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture* texture = SDL_CreateGPUTexture(gpu_device, &texture_info);

    {
        SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(gpu_device);

        transfer_to_gpu(gpu_device, cmd_buf, vertex_buffer, vertices, 3, 0);
        gpu_cube = cube.create_on_gpu(gpu_device, cmd_buf);
        transfer_to_gpu(gpu_device, cmd_buf, texture, texture_surface);

        SDL_SubmitGPUCommandBuffer(cmd_buf);
    }

    SDL_DestroySurface(texture_surface);

    Camera cam{glm::vec3{0.0f, 10.0f, 10.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, -1.0f, 0.0f}, 90.0f,
               16.0f / 10.0f};
    float sensitivity = 1.0f;
    bool lock_cam = false;
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
            bool process_event = lock_cam || !cam.is_freecam();
            if (process_event) ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if ((!process_event || !io.WantCaptureKeyboard) &&
                (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)) {
                ih.update(event.key);
            } else if ((!process_event || !io.WantCaptureMouse) && event.type == SDL_EVENT_MOUSE_MOTION) {
                ih.update(event.motion);
            }
        }

        while (accum >= 1000 / 60) {
            accum -= 1000 / 60;

            if (ih.was_released(SDL_SCANCODE_L)) {
                lock_cam = !lock_cam;
            }
            if (ih.was_released(SDL_SCANCODE_F)) {
                cam.enable_freecam();
                lock_cam = false;
            }

            glm::vec3 movement{0.0f};
            if (ih.is_held(SDL_SCANCODE_W)) {
                movement.z -= 1.0f;
            }
            if (ih.is_held(SDL_SCANCODE_S)) {
                movement.z += 1.0f;
            }
            if (ih.is_held(SDL_SCANCODE_A)) {
                movement.x -= 1.0f;
            }
            if (ih.is_held(SDL_SCANCODE_D)) {
                movement.x += 1.0f;
            }

            bool moved = movement.x != 0.0f || movement.z != 0.0f;
            if (cam.is_freecam()) {
                auto fcam = cam.get_freecam_data();
                if (moved) {
                    auto normalized_movement = glm::normalize(movement);
                    cam.set_position(cam.get_position() + -normalized_movement.z * fcam.forward +
                                     normalized_movement.x * fcam.right);
                }

                if (!lock_cam && (ih.mouse_delta.x != 0.0f || ih.mouse_delta.y != 0.0f)) {
                    cam.set_freecam(fcam.yaw - ih.mouse_delta.x * sensitivity,
                                    glm::clamp(fcam.pitch + ih.mouse_delta.y * sensitivity, -89.0f, 89.0f));
                }
            } else if (moved) {
                cam.set_position(cam.get_position() + glm::normalize(movement));
            }

            ih.update_tick();

            SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(gpu_device);
            transfer_to_gpu(gpu_device, cmd_buf, vertex_buffer, vertices, 3, 0);
            SDL_SubmitGPUCommandBuffer(cmd_buf);

            if (!lock_cam && cam.is_freecam()) {
                SDL_SetWindowRelativeMouseMode(window, true);
            } else {
                SDL_SetWindowRelativeMouseMode(window, false);
            }
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
        if (!lock_cam && cam.is_freecam()) ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

        ImGui::ShowDemoWindow();

        {
            ImGui::Begin("Debugger");

            ImGui::SeparatorText("Camera: Main");
            ImGui::SliderFloat("sensitivity", &sensitivity, 0.0f, 1.0f);
            ImGui::Checkbox("locked", &lock_cam);

            ImGui::SeparatorText("Triangle UVs");
            ImGui::SliderFloat2("Tip", &vertices[0].u, 0.0f, 1.0f);
            ImGui::SliderFloat2("Left", &vertices[1].u, 0.0f, 1.0f);
            ImGui::SliderFloat2("Right", &vertices[2].u, 0.0f, 1.0f);

            ImGui::End();
        }

        cam.imgui_window("Main");

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
        SDL_GPUTextureSamplerBinding sampler_binding{
            .texture = texture,
            .sampler = sampler,
        };
        SDL_BindGPUFragmentSamplers(render_pass, 0, &sampler_binding, 1);

        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

        SDL_BindGPUGraphicsPipeline(render_pass, cube_pipeline);
        SDL_GPUBufferBinding cube_vertex_buf_binding{
            .buffer = gpu_cube.vertex_buffer,
            .offset = 0,
        };
        SDL_GPUBufferBinding cube_index_buf_binding{
            .buffer = gpu_cube.index_buffer,
            .offset = 0,
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &cube_vertex_buf_binding, 1);
        SDL_BindGPUIndexBuffer(render_pass, &cube_index_buf_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        CubeUniformBuffer cube_uniform_buffer{cam.get_view_projection(), cube_color};
        SDL_PushGPUVertexUniformData(command_buffer, 0, &cube_uniform_buffer, sizeof(CubeUniformBuffer));
        SDL_DrawGPUIndexedPrimitives(render_pass, static_cast<uint32_t>(cube.index_count), 1, 0, 0, 0);

        if (!is_minimized) ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_WaitForGPUIdle(gpu_device);

    gpu_cube.release(gpu_device), SDL_ReleaseGPUShader(gpu_device, cube_ver_shader);
    SDL_ReleaseGPUShader(gpu_device, cube_frag_shader);
    SDL_ReleaseGPUGraphicsPipeline(gpu_device, cube_pipeline);

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
