#pragma once

#include <volk.h>

#include <cstdint>

namespace engine::transport {
    void begin();
    uint64_t end();

    // `barrer` needs dstStageMask and dstAccessMask set
    void upload(VkBufferMemoryBarrier2* barrier, void* src, uint32_t size, VkBuffer dst, uint32_t dst_start_offset = 0);

    // `barrer` needs dstStageMask, dstAccessMask and oldLayout set and oldLayout to be a layout
    // that allows for transfer or VK_IMAGE_LAYOUT_UNDEFINED
    void upload(VkImageMemoryBarrier2* barrier, void* src, uint32_t size, uint32_t width, uint32_t height,
                VkFormat format, VkImage dst);
}
