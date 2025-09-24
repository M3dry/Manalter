#pragma once

#include "engine/engine.hpp"
#include <volk.h>

#include <array>
#include <cstdint>
#include <span>

namespace engine {
    class DescriptorPool {
      public:
        using id = uint64_t;

        DescriptorPool();
        ~DescriptorPool();

        uint64_t new_set(VkDescriptorSetLayout layout);
        void bind_set(uint64_t id, VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point, VkPipelineLayout layout,
                      uint32_t set);
        void update_set(uint64_t id, std::span<VkWriteDescriptorSet> writes);

        void begin_update(uint64_t id);
        void end_update();

        void update_ubo(uint32_t binding, std::span<uint8_t> ubo);
        void update_sampled_image(uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler);

        void clear();

      private:
        static constexpr std::size_t MaxSets = 500;
        static constexpr VkDeviceSize UBOSize = 16000;

        VkDescriptorPool pool;

        uint64_t set_count = 0;
        std::array<VkDescriptorSet, MaxSets> sets;

        bool ubo_coherent = false;
        uint64_t ubo_offset = 0;
        uint8_t* ubo_data;
        VkBuffer ubo_buffer;
        VmaAllocation ubo_alloc;

        uint64_t write_id = (std::size_t)-1;
        std::vector<VkDescriptorBufferInfo> write_buffer_infos;
        std::vector<VkDescriptorImageInfo> write_image_infos;
        std::vector<VkWriteDescriptorSet> write_queue;
    };
}
