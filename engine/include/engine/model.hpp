#pragma once

#include "engine/camera.hpp"
#include "engine/util.hpp"

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <glm/ext/matrix_float4x4.hpp>

namespace engine {
    MAKE_WRAPPED_ID(ModelId);
    MAKE_WRAPPED_ID(GPUGroupId);
}

namespace engine::model {
    GPUGroupId new_group();
    bool upload_to_gpu(GPUGroupId group_id, SDL_GPUCommandBuffer* cmd_buf);
    void free_from_gpu(GPUGroupId group_id);

    std::expected<ModelId, std::string> load(GPUGroupId group_id, const char* file_path, bool binary,
                                             std::string* warning = nullptr);
    std::expected<ModelId, std::string> load(GPUGroupId group_id, std::span<std::byte> data, bool binary,
                                             std::string* warning = nullptr);
    void dump_data(ModelId model_id);
    void dump_data(GPUGroupId gpugroup_id);

    bool draw_model(ModelId model_id, glm::mat4 model, const CamId& cam_id, SDL_GPUCommandBuffer* cmd_buf,
                    SDL_GPURenderPass* render_pass);
}
