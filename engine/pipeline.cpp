#include "engine/pipeline.hpp"
#include "engine/gpu.hpp"
#include "pipeline_c.hpp"
#include <SDL3/SDL_gpu.h>

namespace engine::pipeline {
    multi_vector<PipelineDesc, SDL_GPUGraphicsPipeline*> pipelines{};

    PipelineId Make::create() {
        return create_pipeline(*this);
    }

    PipelineId create_pipeline(const Make& info) {
        PipelineDesc desc{info};
        if (auto exists = pipeline_exists(desc); exists) {
            return *exists;
        }

        auto [out_desc, out_pipeline] = pipelines.unsafe_push();

        new (out_desc) PipelineDesc(std::move(desc));
        *out_pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &info._sdl_pipeline);

        return PipelineId{pipelines.size() - 1, *out_pipeline};
    }

    std::optional<PipelineId> pipeline_exists(const PipelineDesc& desc) {
        auto [descs, ptrs] = pipelines.get_span<PipelineDesc, SDL_GPUGraphicsPipeline*>();
        for (std::size_t i = 0; i < descs.size(); i++) {
            if (descs[i] == desc) return PipelineId{i, ptrs[i]};
        }

        return std::nullopt;
    }
}
