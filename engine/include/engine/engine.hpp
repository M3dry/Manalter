#pragma once

#include <SDL3/SDL.h>

#define VK_NO_PROTOTYPES
#include <vk_mem_alloc.h>
#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vector>

#define VK_CHECK(x)                                                                                                    \
    do {                                                                                                               \
        VkResult err = x;                                                                                              \
        if (err) {                                                                                                     \
            printf("Detected Vulkan error: %s @%d\n", string_VkResult(err), __LINE__);                                 \
            exit(-1);                                                                                                  \
        }                                                                                                              \
    } while (0)

namespace engine {
    extern SDL_Window* window;

    extern VkInstance instance;
    extern VkPhysicalDevice physical_device;
    extern VkPhysicalDeviceProperties physical_device_properties;
    extern VkDevice device;
    extern VmaAllocator allocator;
    extern VkSurfaceKHR surface;

    constexpr VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    extern VkExtent2D swapchain_extent;
    extern VkSwapchainKHR swapchain;
    extern std::vector<VkImage> swapchain_images;
    extern std::vector<VkImageView> swapchain_image_views;

    extern VkQueue graphics_queue;
    extern uint32_t graphics_queue_family;
    extern VkQueue transport_queue;
    extern uint32_t transport_queue_family;

    void init(const char* window_name);
    void destroy();

    VkCommandBuffer get_cmd_buf();
    void prepare_frame();
    void prepare_draw();
    void next_frame();
}
