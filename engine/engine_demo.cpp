#include "engine/buffer.hpp"
#include "engine/camera.hpp"
#include "engine/engine.hpp"
#include "engine/gpu.hpp"
#include "engine/imgui.hpp"
#include "engine/input.hpp"
#include "engine/model.hpp"
#include "engine/pipeline.hpp"
#include "engine/shader.hpp"
#include "engine/window.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_timer.h>
#include <SDL3_image/SDL_image.h>
#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>
#include <iostream>
#include <mutex>
#include <unistd.h>

int main(int argc, char** argv) {
    engine::init(engine::InitConfig{
        .window_name = "Engine demo",
    });
    {
        std::string warn{};
        auto gpu_group = engine::model::new_group();
        auto model_ = engine::model::load(gpu_group, argv[1], true, &warn);
        if (model_) {
            std::println("id: {}", ecs::Entity(*model_));
        } else {
            std::println("err: {}", model_.error());
            exit(-1);
        }

        {
            SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
            engine::model::upload_to_gpu(gpu_group, cmd_buf);
            SDL_SubmitGPUCommandBufferAndAcquireFence(cmd_buf);
        }

        engine::model::dump_data(*model_);
        engine::model::dump_data(gpu_group);

        engine::InputHandler ih;
        auto cam = engine::camera::perspective_camera(engine::camera::Perspective{
            .fov = 50.0f,
            .aspect_ratio = 16.0f / 10.0f,
            .near = 0.1f,
            .far = 100.0f,
        });
        engine::camera::set_position(cam, glm::vec3{0.0f, 10.0f, 10.0f});
        engine::camera::look_at(cam, glm::vec3{0.0f});
        bool lock_cam = true;
        float sensitivity = 0.5f;

        auto vertex_shader = engine::shader::Make()
                                 .uniforms(2)
                                 .storage_buffers(1)
                                 .stage(engine::shader::Vertex)
                                 .load_from_file("./model_vertex.spv");
        auto fragment_shader = engine::shader::Make()
                                   .uniforms(3)
                                   .samplers(5)
                                   .stage(engine::shader::Fragment)
                                   .load_from_file("./model_fragment.spv");
        auto pipeline_info = engine::pipeline::Make()
                                 .shaders(vertex_shader, fragment_shader)
                                 .vertex_buffers({})
                                 .vertex_attributes({})
                                 .primitive(engine::pipeline::TriangleList)
                                 .fill_mode(engine::pipeline::Fill)
                                 .cull_mode(engine::pipeline::CullNone)
                                 .front_face(engine::pipeline::CCW)
                                 .no_depth_bias()
                                 .no_depth_clip()
                                 .depth_stencil_format(SDL_GPU_TEXTUREFORMAT_D16_UNORM)
                                 .depth_write()
                                 .depth_testing(engine::pipeline::Less)
                                 .color_targets({engine::pipeline::ColorTarget().format(
                                     SDL_GetGPUSwapchainTextureFormat(engine::gpu_device, engine::win))});
        auto pipeline = pipeline_info.create();

        engine::Buffer vertex_buffer{sizeof(glm::vec3) * 3, engine::buffer::Vertex};
        vertex_buffer.name("Triangle vertices");

        {
            glm::vec3 triangle_vertices[] = {
                {3.0f, 0.0f, 3.0f},
                {-3.0f, 0.0f, 3.0f},
                {0.0f, 0.0f, -3.0f},
            };

            SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
            engine::gpu::transfer_to_gpu(cmd_buf, vertex_buffer, triangle_vertices, sizeof(glm::vec3) * 3, 0);
            SDL_SubmitGPUCommandBuffer(cmd_buf);
        }

        SDL_GPUTextureCreateInfo depth_texture_info{
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .width = 1920,
            .height = 1200,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        };
        SDL_GPUTexture* depth_texture = SDL_CreateGPUTexture(engine::gpu_device, &depth_texture_info);

        glm::vec3 light_pos{5.0f, 5.0f, 5.0};
        glm::vec3 light_color{1000.0f, 1000.0f, 1000.0f};

        bool done = false;
        uint64_t accum = 0;
        uint64_t last_time = SDL_GetTicks();
        while (!done) {
            uint64_t time = SDL_GetTicks();
            uint64_t frame_time = time - last_time;
            last_time = time;
            accum += frame_time;

            done = engine::poll_input(ih);

            while (accum >= 1000 / 60) {
                accum -= 1000 / 60;

                if (ih.was_released(SDL_SCANCODE_L)) {
                    lock_cam = !lock_cam;
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
                if (moved) {
                    auto forward = engine::camera::forward(cam);
                    auto right = engine::camera::right(cam);

                    auto normalized_movement = glm::normalize(movement);
                    engine::camera::move(cam, -normalized_movement.z * forward + normalized_movement.x * right);
                }
                auto mouse_delta = ih.get_mouse_delta();
                if (!lock_cam && (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f)) {
                    engine::camera::rotate(cam, -mouse_delta.x * sensitivity, -mouse_delta.y * sensitivity);
                }

                ih.update_tick();

                engine::imgui::enable(lock_cam);
                SDL_SetWindowRelativeMouseMode(engine::win, !lock_cam);
            }

            if (engine::window::is_minimized()) {
                SDL_Delay(10);
                continue;
            }

            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
            SDL_GPUTexture* swapchain_texture;
            SDL_AcquireGPUSwapchainTexture(command_buffer, engine::win, &swapchain_texture, nullptr, nullptr);
            if (swapchain_texture == nullptr) {
                SDL_SubmitGPUCommandBuffer(command_buffer);
                continue;
            }

            engine::imgui::new_frame();
            engine::camera::full_imgui_window();
            {
                ImGui::Begin("Debug");
                ImGui::SeparatorText("Global camera config");
                ImGui::DragFloat("sensitivity", &sensitivity, 0.01f, 0.0f, 1.0f, "%.2f");

                ImGui::SeparatorText("Light");
                ImGui::DragFloat3("position", glm::value_ptr(light_pos), 0.1f, -10.0f, 10.0f, "%.1f");
                ImGui::InputFloat3("color", glm::value_ptr(light_color));
                ImGui::End();
            }
            engine::imgui::prepare_data(command_buffer);

            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor{0.f, 0.f, 0.f, 1.f};
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;

            SDL_GPUDepthStencilTargetInfo depth_stencil_info{
                .texture = depth_texture,
                .clear_depth = 1,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
                .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
                .stencil_store_op = SDL_GPU_STOREOP_STORE,
                .cycle = true,
                .clear_stencil = 1,
            };

            SDL_GPURenderPass* render_pass =
                SDL_BeginGPURenderPass(command_buffer, &target_info, 1, &depth_stencil_info);

            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            glm::vec4 light_data[2] = {glm::vec4(light_pos, 1.0), glm::vec4(light_color, 1.0)};
            SDL_PushGPUFragmentUniformData(command_buffer, 2, &light_data, 2*sizeof(glm::vec4));
            engine::model::draw_model(*model_, glm::identity<glm::mat4>(), cam, command_buffer, render_pass);

            engine::imgui::draw(command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
            SDL_SubmitGPUCommandBuffer(command_buffer);
        }
    }
    engine::deinit();
    return 0;
}
