#pragma once

#include <volk.h>

#include <cstdint>
#include <span>

namespace engine::descriptor {
    uint64_t new_set(VkDescriptorSetLayout layout);
    void bind_set(uint64_t id, VkPipelineBindPoint bind_point, VkPipelineLayout layout, uint32_t set);
    void update_set(uint64_t id, std::span<VkWriteDescriptorSet> writes);

    void begin_update(uint64_t id);
    void end_update();

    void update_ubo(uint32_t binding, std::span<uint8_t> ubo);
    void update_sampled_image(uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler);
}
