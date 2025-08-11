#pragma once

#include "engine/util.hpp"

#include <SDL3/SDL_gpu.h>
#include <expected>

namespace engine {
    MAKE_WRAPPED_ID(ModelId);
    MAKE_WRAPPED_ID(MeshId);

    MAKE_WRAPPED_ID(GPUMeshId);
}

namespace engine::model {
    std::expected<ModelId, std::string> load(const char* file_path, bool binary, std::string* warning = nullptr);
    std::expected<ModelId, std::string> load(std::span<std::byte> data, bool binary, std::string* warning = nullptr);
    void dump_data(ModelId model_id);

    void upload_to_gpu(SDL_GPUCommandBuffer* cmd_buf);
    bool draw_model(ModelId model_id, SDL_GPUCommandBuffer* cmd_buf, SDL_GPURenderPass* render_pass,
                    uint32_t uniform_slot, uint32_t buffer_slot);
}
