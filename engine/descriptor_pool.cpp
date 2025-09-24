#include "engine/descriptor_pool.hpp"
#include "descriptor_pool_.hpp"
#include "engine/engine.hpp"
#include "engine_.hpp"

#include <cassert>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace engine {
    DescriptorPool::DescriptorPool() {
        VkDescriptorPoolSize pool_sizes[4];
        pool_sizes[0].descriptorCount = MaxSets;
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount = MaxSets;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[2].descriptorCount = MaxSets;
        pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[3].descriptorCount = MaxSets;
        pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = 4;
        pool_info.pPoolSizes = pool_sizes;
        pool_info.maxSets = MaxSets;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

        VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool));

        VkBufferCreateInfo buf_info{};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_info.size = UBOSize;
        buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo out_alloc_info;
        VK_CHECK(vmaCreateBuffer(allocator, &buf_info, &alloc_info, &ubo_buffer, &ubo_alloc, &out_alloc_info));

        VkMemoryPropertyFlags alloc_props;
        vmaGetMemoryTypeProperties(allocator, out_alloc_info.memoryType, &alloc_props);

        ubo_data = (uint8_t*)out_alloc_info.pMappedData;
        ubo_coherent = alloc_props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(device, pool, nullptr);
        vmaDestroyBuffer(allocator, ubo_buffer, ubo_alloc);
    }

    uint64_t DescriptorPool::new_set(VkDescriptorSetLayout layout) {
        assert(set_count < MaxSets);

        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = pool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &layout;

        VK_CHECK(vkAllocateDescriptorSets(device, &info, &sets[set_count]));

        return set_count++;
    }

    void DescriptorPool::bind_set(uint64_t id, VkCommandBuffer cmd_buf, VkPipelineBindPoint bind_point,
                                  VkPipelineLayout layout, uint32_t set) {
        vkCmdBindDescriptorSets(cmd_buf, bind_point, layout, set, 1, &sets[id], 0, nullptr);
    }

    void DescriptorPool::update_set(uint64_t id, std::span<VkWriteDescriptorSet> writes) {
        assert(id < MaxSets);

        for (auto& write : writes) {
            write.dstSet = sets[id];
        }

        vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
    }

    void DescriptorPool::begin_update(uint64_t id) {
        assert(id < MaxSets);
        write_id = id;

        write_buffer_infos.clear();
        write_image_infos.clear();
        write_queue.clear();
    }

    void DescriptorPool::end_update() {
        for (auto& write : write_queue) {
            switch (write.descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    write.pBufferInfo = &write_buffer_infos[(std::size_t)write.pBufferInfo];
                    break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    write.pImageInfo = &write_image_infos[(std::size_t)write.pImageInfo];
                    break;
                default: continue;
            }
        }

        update_set(write_id, write_queue);
    }

    void DescriptorPool::update_ubo(uint32_t binding, std::span<uint8_t> ubo) {
        assert(write_id != (uint64_t)-1);
        assert(ubo_offset + ubo.size() <= UBOSize);

        std::memcpy(ubo_data + ubo_offset, ubo.data(), ubo.size());
        if (!ubo_coherent) {
            vmaFlushAllocation(allocator, ubo_alloc, ubo_offset, ubo.size());
        }

        VkDescriptorBufferInfo buf_info{};
        buf_info.buffer = ubo_buffer;
        buf_info.offset = ubo_offset;
        buf_info.range = ubo.size();

        write_buffer_infos.emplace_back(buf_info);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = (VkDescriptorBufferInfo*)(write_buffer_infos.size() - 1);
        write.dstBinding = binding;
        write.dstSet = sets[write_id];

        write_queue.emplace_back(write);

        ubo_offset += ubo.size();
    }

    void DescriptorPool::update_sampled_image(uint32_t binding, VkImageLayout layout, VkImageView view,
                                              VkSampler sampler) {
        assert(write_id != (uint64_t)-1);

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = layout;
        image_info.imageView = view;
        image_info.sampler = sampler;

        write_image_infos.emplace_back(image_info);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1;
        write.pImageInfo = (VkDescriptorImageInfo*)(write_image_infos.size() - 1);
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.dstSet = sets[write_id];
        write.dstBinding = binding;

        write_queue.emplace_back(write);
    }

    void DescriptorPool::clear() {
        VK_CHECK(vkResetDescriptorPool(device, pool, 0));

        set_count = 0;
        std::memset(sets.data(), 0, MaxSets * sizeof(VkDescriptorSet));

        ubo_offset = 0;
    }
}

namespace engine::descriptor {
    uint64_t new_set(VkDescriptorSetLayout layout) {
        return get_frame_descriptor_pool().new_set(layout);
    }

    void bind_set(uint64_t id, VkPipelineBindPoint bind_point, VkPipelineLayout layout, uint32_t set) {
        get_frame_descriptor_pool().bind_set(id, get_cmd_buf(), bind_point, layout, set);
    }

    void update_set(uint64_t id, std::span<VkWriteDescriptorSet> writes) {
        get_frame_descriptor_pool().update_set(id, writes);
    }

    void begin_update(uint64_t id) {
        get_frame_descriptor_pool().begin_update(id);
    }

    void end_update() {
        get_frame_descriptor_pool().end_update();
    }

    void update_ubo(uint32_t binding, std::span<uint8_t> ubo) {
        get_frame_descriptor_pool().update_ubo(binding, ubo);
    }

    void update_sampled_image(uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler) {
        get_frame_descriptor_pool().update_sampled_image(binding, layout, view, sampler);
    }
}
