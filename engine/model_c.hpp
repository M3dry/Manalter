#pragma once

#include "engine/util.hpp"
#include "util_c.hpp"
#include <SDL3/SDL_gpu.h>
#include <memory>

namespace engine::model {
    MAKE_WRAPPED_ID(MeshStorageBufId);

    struct MeshIndices {
        // == -1 implies no index buffer, use mesh_vertex_count_tag w/o indexing
        SDL_GPUIndexElementSize stride;
        size_t count;
        array_unique_ptr<std::byte> indices;

        MeshIndices() {};

        MeshIndices(MeshIndices&&) noexcept = default;
        MeshIndices& operator=(MeshIndices&&) noexcept = default;
    };

    struct GPUOffsets {
        enum Attribute {
            Indices = 1 << 0,
            Position = 1 << 1,
            Normal = 1 << 2,
        };

        uint32_t start_offset;
        uint32_t indices_offset;
        uint32_t position_offset;
        uint32_t normal_offset;
        uint32_t stride;
        uint32_t attribute_mask;
    };
}
