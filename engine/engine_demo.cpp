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

struct CubeVertexUniforms {
    glm::mat4 vp;
    glm::mat4 model;
};

struct alignas(16) CubeFragmentUniforms {
    glm::vec4 color;
    glm::vec4 light_color;
    glm::vec4 light_position;
    glm::vec4 view_position;
    glm::vec3 info;
};

struct LightUniforms {
    glm::mat4 mvp;
    glm::vec4 color;
};

struct Vertex {
    float x, y, z;
    float norm_x, norm_y, norm_z;
};

float vertices[] = {-0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,
                    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
                    -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

                    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,
                    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
                    -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

                    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,
                    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
                    -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

                    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,
                    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
                    0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

                    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,
                    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
                    -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

                    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,
                    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
                    -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f};

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
            SDL_SubmitGPUCommandBuffer(cmd_buf);
        }

        engine::model::dump_data(*model_);

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

        auto* vertex_shader = engine::shader::Make()
                                  .uniforms(1)
                                  .stage(engine::shader::Vertex)
                                  .load_from_file("./model_vertex2.spv");
        auto* fragment_shader =
            engine::shader::Make().stage(engine::shader::Fragment).load_from_file("./model_fragment.spv");
        auto* pipeline = engine::pipeline::Make()
                             .shaders(vertex_shader, fragment_shader)
                             .vertex_buffers({SDL_GPUVertexBufferDescription{
                                 .slot = 0,
                                 .pitch = sizeof(glm::vec3) + sizeof(glm::vec3),
                                 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                                 .instance_step_rate = 0,
                             }})
                             .vertex_attributes({SDL_GPUVertexAttribute{
                                                     .location = 0,
                                                     .buffer_slot = 0,
                                                     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                                     .offset = 0,
                                                 },
                                                 SDL_GPUVertexAttribute{
                                                     .location = 1,
                                                     .buffer_slot = 0,
                                                     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                                                     .offset = sizeof(float) * 3,
                                                 }})
                             .primitive(engine::pipeline::TriangleList)
                             .fill_mode(engine::pipeline::Fill)
                             .cull_mode(engine::pipeline::CullNone)
                             .front_face(engine::pipeline::CCW)
                             .no_depth_bias()
                             .no_depth_clip()
                             .depth_stencil_format(SDL_GPU_TEXTUREFORMAT_D16_UNORM)
                             .depth_write()
                             .color_targets({engine::pipeline::ColorTarget().format(
                                 SDL_GetGPUSwapchainTextureFormat(engine::gpu_device, engine::win))})
                             .create();

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
            glm::mat4 vp = engine::camera::view_projection_matrix(cam);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &vp, sizeof(glm::mat4));

            SDL_GPUBufferBinding buffer_binding{
                .buffer = vertex_buffer,
                .offset = 0,
            };
            SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
            engine::model::draw_model(*model_, command_buffer, render_pass, 1, 0);
            // SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

            engine::imgui::draw(command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
            SDL_SubmitGPUCommandBuffer(command_buffer);
        }

        // auto* vertex_shader =
        //     engine::shader::Make().uniforms(1).stage(engine::shader::Vertex).load_from_file("./vertex.spv");
        // auto* fragment_shader =
        //     engine::shader::Make().uniforms(1).stage(engine::shader::Fragment).load_from_file("./fragment.spv");
        //
        // auto* light_vertex_shader =
        //     engine::shader::Make().uniforms(1).stage(engine::shader::Vertex).load_from_file("./light_vertex.spv");
        // auto* light_fragment_shader =
        //     engine::shader::Make().stage(engine::shader::Fragment).load_from_file("./light_fragment.spv");
        //
        // SDL_GPUGraphicsPipeline* pipeline =
        //     engine::pipeline::Make()
        //         .shaders(vertex_shader, fragment_shader)
        //         .vertex_buffers({engine::pipeline::VertexBuffer<glm::vec3, glm::vec3>{0}})
        //         .vertex_attributes(engine::pipeline::VertexAttributes<glm::vec3, glm::vec3>{0})
        //         .primitive(engine::pipeline::TriangleList)
        //         .fill_mode(engine::pipeline::Fill)
        //         .cull_mode(engine::pipeline::CullNone)
        //         .front_face(engine::pipeline::CCW)
        //         .no_depth_bias()
        //         .no_depth_clip()
        //         .depth_stencil_format(SDL_GPU_TEXTUREFORMAT_D16_UNORM)
        //         .depth_testing(engine::pipeline::Less)
        //         .depth_write()
        //         .color_targets({engine::pipeline::ColorTarget().blend_state({}).format(
        //             SDL_GetGPUSwapchainTextureFormat(engine::gpu_device, engine::win))})
        //         .create();
        // SDL_GPUGraphicsPipeline* light_pipeline =
        //     engine::pipeline::Make()
        //         .shaders(light_vertex_shader, light_fragment_shader)
        //         .vertex_buffers({engine::pipeline::VertexBuffer<glm::vec3, glm::vec3>{0}})
        //         .vertex_attributes(engine::pipeline::VertexAttributes<glm::vec3,
        //         engine::pipeline::blank<glm::vec3>>{0}) .primitive(engine::pipeline::TriangleList)
        //         .fill_mode(engine::pipeline::Fill)
        //         .cull_mode(engine::pipeline::CullNone)
        //         .front_face(engine::pipeline::CCW)
        //         .no_depth_bias()
        //         .no_depth_clip()
        //         .depth_stencil_format(SDL_GPU_TEXTUREFORMAT_D16_UNORM)
        //         .depth_testing(engine::pipeline::Less)
        //         .depth_write()
        //         .color_targets({engine::pipeline::ColorTarget().blend_state({}).format(
        //             SDL_GetGPUSwapchainTextureFormat(engine::gpu_device, engine::win))})
        //         .create();
        //
        // engine::Buffer vertex_buffer{sizeof(vertices), engine::buffer::Vertex};
        // vertex_buffer.name("Cube vertices");
        //
        // SDL_GPUTextureCreateInfo depth_texture_info{
        //     .type = SDL_GPU_TEXTURETYPE_2D,
        //     .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        //     .width = 1920,
        //     .height = 1200,
        //     .layer_count_or_depth = 1,
        //     .num_levels = 1,
        //     .sample_count = SDL_GPU_SAMPLECOUNT_1,
        //     .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        // };
        // SDL_GPUTexture* depth_texture = SDL_CreateGPUTexture(engine::gpu_device, &depth_texture_info);
        //
        // {
        //     SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
        //     engine::gpu::transfer_to_gpu(cmd_buf, vertex_buffer, vertices, sizeof(vertices), 0);
        //     SDL_SubmitGPUCommandBuffer(cmd_buf);
        // }
        //
        // float cube_scale = 1.0f;
        // glm::vec3 cube_rotation{0.0f};
        // glm::vec3 cube_position{0.0f, 0.0f, 0.0f};
        // glm::mat4 cube_model = glm::identity<glm::mat4>();
        // glm::vec4 cube_color{1.0f, 0.0f, 0.0f, 1.0f};
        //
        // float light_scale = 0.5f;
        // glm::vec3 light_rotation{0.0f};
        // glm::vec3 light_position{5.0f, 5.0f, 5.0f};
        // glm::mat4 light_model = glm::identity<glm::mat4>();
        // glm::vec4 light_color{1.0f, 1.0f, 1.0f, 1.0f};
        //
        // float specular_strength = 0.5f;
        // float shininess = 69;
        // float ambient_strength = 0.1f;
        //
        // bool lock_cam = true;
        // float sensitivity = 0.5f;
        //
        // bool done = false;
        // uint64_t accum = 0;
        // uint64_t last_time = SDL_GetTicks();
        // while (!done) {
        //     uint64_t time = SDL_GetTicks();
        //     uint64_t frame_time = time - last_time;
        //     last_time = time;
        //     accum += frame_time;
        //
        //     done = engine::poll_input(ih);
        //
        //     while (accum >= 1000 / 60) {
        //         accum -= 1000 / 60;
        //
        //         if (ih.was_released(SDL_SCANCODE_L)) {
        //             lock_cam = !lock_cam;
        //         }
        //
        //         glm::vec3 movement{0.0f};
        //         if (ih.is_held(SDL_SCANCODE_W)) {
        //             movement.z -= 1.0f;
        //         }
        //         if (ih.is_held(SDL_SCANCODE_S)) {
        //             movement.z += 1.0f;
        //         }
        //         if (ih.is_held(SDL_SCANCODE_A)) {
        //             movement.x -= 1.0f;
        //         }
        //         if (ih.is_held(SDL_SCANCODE_D)) {
        //             movement.x += 1.0f;
        //         }
        //
        //         bool moved = movement.x != 0.0f || movement.z != 0.0f;
        //         if (moved) {
        //             auto forward = engine::camera::forward(cam);
        //             auto right = engine::camera::right(cam);
        //
        //             auto normalized_movement = glm::normalize(movement);
        //             engine::camera::move(cam, -normalized_movement.z * forward + normalized_movement.x * right);
        //         }
        //         auto mouse_delta = ih.get_mouse_delta();
        //         if (!lock_cam && (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f)) {
        //             engine::camera::rotate(cam, -mouse_delta.x * sensitivity, -mouse_delta.y * sensitivity);
        //         }
        //
        //         ih.update_tick();
        //
        //         engine::imgui::enable(lock_cam);
        //         SDL_SetWindowRelativeMouseMode(engine::win, !lock_cam);
        //     }
        //
        //     if (engine::window::is_minimized()) {
        //         SDL_Delay(10);
        //         continue;
        //     }
        //
        //     SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(engine::gpu_device);
        //     SDL_GPUTexture* swapchain_texture;
        //     SDL_AcquireGPUSwapchainTexture(command_buffer, engine::win, &swapchain_texture, nullptr, nullptr);
        //     if (swapchain_texture == nullptr) {
        //         SDL_SubmitGPUCommandBuffer(command_buffer);
        //         continue;
        //     }
        //
        //     engine::imgui::new_frame();
        //     ImGui::ShowDemoWindow();
        //     engine::camera::full_imgui_window();
        //     {
        //         ImGui::Begin("Models");
        //         ImGui::SeparatorText("cube");
        //         ImGui::DragFloat("scale", &cube_scale, 0.1f, 1.0f, 10.0f, "%.1f");
        //         ImGui::DragFloat3("rotation", glm::value_ptr(cube_rotation), 0.1f, 0.0f, 360.0f, "%.1f");
        //         ImGui::InputFloat3("position", glm::value_ptr(cube_position), "%.0f");
        //         ImGui::ColorPicker3("color", glm::value_ptr(cube_color));
        //
        //         ImGui::SeparatorText("light");
        //         ImGui::PushID(2);
        //         ImGui::DragFloat("scale", &light_scale, 0.05f, 0.1f, 5.0f, "%.2f");
        //         ImGui::DragFloat3("rotation", glm::value_ptr(light_rotation), 0.1f, 0.0f, 360.0f, "%.1f");
        //         ImGui::InputFloat3("position", glm::value_ptr(light_position), "%.0f");
        //         ImGui::ColorPicker3("color", glm::value_ptr(light_color));
        //         ImGui::PopID();
        //
        //         ImGui::SeparatorText("Lightning");
        //         ImGui::DragFloat("specular strength", &specular_strength, 0.01f, 0.0f, 1.0f, "%.2f");
        //         ImGui::DragFloat("shininess", &shininess, 1, 0, 256);
        //         ImGui::DragFloat("ambient strength", &ambient_strength, 0.01f, 0.0f, 1.0f, "%.2f");
        //
        //         ImGui::SeparatorText("Global camera config");
        //         ImGui::DragFloat("sensitivity", &sensitivity, 0.01f, 0.0f, 1.0f, "%.2f");
        //         ImGui::End();
        //
        //         auto x =
        //             glm::rotate(glm::identity<glm::mat4>(), glm::radians(cube_rotation.x), glm::vec3{1.0f, 0.0f,
        //             0.0f});
        //         auto y = glm::rotate(x, glm::radians(cube_rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
        //         auto z = glm::rotate(y, glm::radians(cube_rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
        //         cube_model =
        //             glm::scale(glm::translate(glm::identity<glm::mat4>(), cube_position), glm::vec3{cube_scale})
        //             * z;
        //
        //         x = glm::rotate(glm::identity<glm::mat4>(), glm::radians(light_rotation.x),
        //                         glm::vec3{1.0f, 0.0f, 0.0f});
        //         y = glm::rotate(x, glm::radians(light_rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
        //         z = glm::rotate(y, glm::radians(light_rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
        //         light_model =
        //             glm::scale(glm::translate(glm::identity<glm::mat4>(), light_position),
        //             glm::vec3{light_scale}) * z;
        //     }
        //     engine::imgui::prepare_data(command_buffer);
        //
        //     SDL_GPUColorTargetInfo target_info = {};
        //     target_info.texture = swapchain_texture;
        //     target_info.clear_color = SDL_FColor{0.f, 0.f, 0.f, 1.f};
        //     target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        //     target_info.store_op = SDL_GPU_STOREOP_STORE;
        //     target_info.mip_level = 0;
        //     target_info.layer_or_depth_plane = 0;
        //     target_info.cycle = false;
        //
        //     SDL_GPUDepthStencilTargetInfo depth_stencil_info{
        //         .texture = depth_texture,
        //         .clear_depth = 1,
        //         .load_op = SDL_GPU_LOADOP_CLEAR,
        //         .store_op = SDL_GPU_STOREOP_STORE,
        //         .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
        //         .stencil_store_op = SDL_GPU_STOREOP_STORE,
        //         .cycle = true,
        //         .clear_stencil = 1,
        //     };
        //
        //     SDL_GPURenderPass* render_pass =
        //         SDL_BeginGPURenderPass(command_buffer, &target_info, 1, &depth_stencil_info);
        //
        //     SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
        //     CubeVertexUniforms cube_vertex_uniform{engine::camera::view_projection_matrix(cam), cube_model};
        //     SDL_PushGPUVertexUniformData(command_buffer, 0, &cube_vertex_uniform, sizeof(CubeVertexUniforms));
        //     CubeFragmentUniforms cube_fragment_uniform{.color = cube_color,
        //                                                .light_color = light_color,
        //                                                .light_position = glm::vec4{light_position, 1.0f},
        //                                                .view_position =
        //                                                glm::vec4{engine::camera::position(cam), 1.0f}, .info =
        //                                                glm::vec3{specular_strength, shininess,
        //                                                ambient_strength}};
        //     SDL_PushGPUFragmentUniformData(command_buffer, 0, &cube_fragment_uniform,
        //     sizeof(CubeFragmentUniforms)); SDL_GPUBufferBinding buffer_binding[1]{{
        //         .buffer = vertex_buffer,
        //         .offset = 0,
        //     }};
        //     SDL_BindGPUVertexBuffers(render_pass, 0, buffer_binding, 1);
        //     SDL_DrawGPUPrimitives(render_pass, sizeof(vertices) / sizeof(Vertex), 1, 0, 0);
        //
        //     SDL_BindGPUGraphicsPipeline(render_pass, light_pipeline);
        //     LightUniforms light_uniform{engine::camera::view_projection_matrix(cam) * light_model, light_color};
        //     SDL_PushGPUVertexUniformData(command_buffer, 0, &light_uniform, sizeof(LightUniforms));
        //     SDL_BindGPUVertexBuffers(render_pass, 0, buffer_binding, 1);
        //     SDL_DrawGPUPrimitives(render_pass, sizeof(vertices) / sizeof(Vertex), 1, 0, 0);
        //
        //     engine::imgui::draw(command_buffer, render_pass);
        //     SDL_EndGPURenderPass(render_pass);
        //     SDL_SubmitGPUCommandBuffer(command_buffer);
        // }
    }
    engine::deinit();
    return 0;
}
