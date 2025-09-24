#include "engine/engine.hpp"
#include "engine_.hpp"

#define VMA_IMPLEMENTATION 1
#include <vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#include "imgui_.hpp"
#include "transport_.hpp"
#include "vk-bootstrap/src/VkBootstrap.h"

#include <SDL3/SDL_vulkan.h>
#include <print>

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout) {
    VkImageMemoryBarrier2 image_barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_barrier.pNext = nullptr;

    image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    image_barrier.oldLayout = current_layout;
    image_barrier.newLayout = new_layout;

    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                         ? VK_IMAGE_ASPECT_DEPTH_BIT
                                         : VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange = VkImageSubresourceRange{};
    image_barrier.subresourceRange.aspectMask = aspect_mask;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    image_barrier.image = image;

    VkDependencyInfo dep_info{};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.pNext = nullptr;

    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &image_barrier;

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

namespace engine {
    SDL_Window* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    VkDevice device;
    VmaAllocator allocator;
    VkSurfaceKHR surface;

    VkExtent2D swapchain_extent;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;

    VkQueue graphics_queue;
    uint32_t graphics_queue_family;
    VkQueue transport_queue;
    uint32_t transport_queue_family;

    FrameData* frames;
    uint8_t current_frame_data = 0;

    uint32_t swapchain_ix;

    FrameData::FrameData() {
        VkCommandPoolCreateInfo cmd_pool_info{};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmd_pool_info.queueFamilyIndex = graphics_queue_family;

        vkCreateCommandPool(device, &cmd_pool_info, nullptr, &cmd_pool);

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.pNext = nullptr;
        cmd_alloc_info.commandPool = cmd_pool;
        cmd_alloc_info.commandBufferCount = 1;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmd_alloc_info, &cmd_buf);

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.pNext = nullptr;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &render_fence));

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;

        VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_semaphore));
    }

    FrameData::~FrameData() {
        vkDestroyCommandPool(device, cmd_pool, nullptr);

        vkDestroyFence(device, render_fence, nullptr);

        vkDestroySemaphore(device, swapchain_semaphore, nullptr);
        vkDestroySemaphore(device, render_semaphore, nullptr);
    }

    void init(const char* window_name) {
        VK_CHECK(volkInitialize());

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::println("sdl error: {}", SDL_GetError());
            exit(-1);
        }

        window = SDL_CreateWindow(window_name, 0, 0,
                                  SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN | SDL_WINDOW_SURFACE_VSYNC_DISABLED);

        vkb::InstanceBuilder instance_builder;
        auto vkb_inst_builder = instance_builder.set_app_name("Vulkan test")
                                    .enable_validation_layers()
                                    .use_default_debug_messenger()
                                    .require_api_version(1, 3, 0)
                                    .build();
        if (!vkb_inst_builder) {
            std::println("vkb error: {}", vkb_inst_builder.error().message());
            exit(-1);
        }

        vkb::Instance vkb_inst = vkb_inst_builder.value();
        instance = vkb_inst.instance;
        volkLoadInstance(instance);

        debug_messenger = vkb_inst.debug_messenger;
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
            std::println("sdl error: {}", SDL_GetError());
            exit(-1);
        }

        VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features13.dynamicRendering = true;
        features13.synchronization2 = true;
        VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;
        features12.shaderSampledImageArrayNonUniformIndexing = true;
        features12.runtimeDescriptorArray = true;
        features12.timelineSemaphore = true;

        auto physical_device_selected = vkb::PhysicalDeviceSelector{vkb_inst}
                                            .set_minimum_version(1, 3)
                                            .set_required_features_13(features13)
                                            .set_required_features_12(features12)
                                            .set_surface(surface)
                                            .add_required_extensions({"VK_EXT_shader_object"})
                                            .select();
        if (!physical_device_selected) {
            std::println("vkb error: {}", physical_device_selected.error().message());
            exit(-1);
        }

        vkb::PhysicalDevice vkb_physical_device = physical_device_selected.value();
        physical_device = vkb_physical_device.physical_device;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

        VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_extension{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
            .pNext = nullptr,
            .shaderObject = true,
        };

        vkb::Device vkb_device =
            vkb::DeviceBuilder{vkb_physical_device}.add_pNext(&shader_object_extension).build().value();
        device = vkb_device.device;

        volkLoadDevice(device);

        VmaVulkanFunctions vma_vulkan_funcs{};
        vma_vulkan_funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vma_vulkan_funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo vma_allocator_info{};
        vma_allocator_info.physicalDevice = physical_device;
        vma_allocator_info.device = device;
        vma_allocator_info.instance = instance;
        vma_allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vma_allocator_info.pVulkanFunctions = &vma_vulkan_funcs;
        vmaCreateAllocator(&vma_allocator_info, &allocator);

        vkb::Swapchain vkb_swapchain =
            vkb::SwapchainBuilder{physical_device, device, surface}
                .set_desired_format(
                    VkSurfaceFormatKHR{.format = swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
                .set_desired_extent(1920, 1200)
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .set_desired_min_image_count(2)
                .build()
                .value();

        swapchain_extent = vkb_swapchain.extent;
        swapchain = vkb_swapchain.swapchain;
        swapchain_images = vkb_swapchain.get_images().value();
        swapchain_image_views = vkb_swapchain.get_image_views().value();

        graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
        graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

        transport_queue = vkb_device.get_queue(vkb::QueueType::transfer).value();
        transport_queue_family = vkb_device.get_queue_index(vkb::QueueType::transfer).value();

        frames = new FrameData[3]{};

        transport::init();
        imgui::init();
    };

    void destroy() {
        vkDeviceWaitIdle(device);

        imgui::destroy();
        transport::destroy();

        delete[] frames;

        vmaDestroyAllocator(allocator);

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        for (std::size_t i = 0; i < swapchain_image_views.size(); i++) {
            vkDestroyImageView(device, swapchain_image_views[i], nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debug_messenger);
        vkDestroyInstance(instance, nullptr);

        SDL_DestroyWindow(window);
    }

    FrameData& get_current_frame_data() {
        return frames[current_frame_data];
    }

    VkCommandBuffer get_cmd_buf() {
        return get_current_frame_data().cmd_buf;
    }

    void prepare_frame() {
        FrameData& frame = get_current_frame_data();
        VK_CHECK(vkWaitForFences(device, 1, &frame.render_fence, true, UINT64_MAX));
        VK_CHECK(vkResetFences(device, 1, &frame.render_fence));

        frame.descriptor_pool.clear();

        auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.swapchain_semaphore, VK_NULL_HANDLE,
                                            &swapchain_ix);
        if (result != VK_SUCCESS) {
            printf("remake the swapchain you lobotomized donkey\n");
        }
    }

    void prepare_draw() {
        VkCommandBuffer cmd_buf = get_cmd_buf();
        vkResetCommandBuffer(cmd_buf, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmd_buf, &begin_info));

        transition_image(cmd_buf, swapchain_images[swapchain_ix], VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    void next_frame() {
        FrameData& frame = get_current_frame_data();

        transition_image(frame.cmd_buf, swapchain_images[swapchain_ix], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(frame.cmd_buf));

        VkSemaphoreSubmitInfo wait_info{};
        wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait_info.semaphore = frame.swapchain_semaphore;
        wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        wait_info.value = 1;

        VkSemaphoreSubmitInfo signal_info{};
        signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal_info.semaphore = frame.render_semaphore;
        signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        signal_info.value = 1;

        VkCommandBufferSubmitInfo cmd_info{};
        cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_info.commandBuffer = frame.cmd_buf;

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.waitSemaphoreInfoCount = 1;
        submit_info.pWaitSemaphoreInfos = &wait_info;
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &cmd_info;
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = &signal_info;

        VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit_info, frame.render_fence));

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &frame.render_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &swapchain_ix;

        VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));

        current_frame_data = (current_frame_data + 1) % 3;
    }

    DescriptorPool& get_frame_descriptor_pool() {
        return get_current_frame_data().descriptor_pool;
    }
}
