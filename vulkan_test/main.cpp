#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <VkBootstrap.h>
#include <cmath>
#include <glm/ext/vector_float4.hpp>
#include <vulkan/vulkan.h>

#include <print>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#define VK_CHECK(x)                                                                                                    \
    do {                                                                                                               \
        VkResult err = x;                                                                                              \
        if (err) {                                                                                                     \
            std::println("Detected Vulkan error: {} @{}", string_VkResult(err), __LINE__);                             \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)

#define FRAMES_IN_FLIGHT 2

struct FrameData {
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buf;
    VkSemaphore swapchain_semaphore;
    VkSemaphore render_semaphore;
    VkFence render_fence;
};

void check_vk_result(VkResult err) {
    if (err == 0) return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0) abort();
}

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

int main(int argc, char** argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("error: {}", SDL_GetError());
        return -1;
    }

    auto* win = SDL_CreateWindow("Vulkan test", 1920, 1200, SDL_WINDOW_VULKAN);

    vkb::InstanceBuilder instance_builder;

    auto vkb_inst_builder = instance_builder.set_app_name("Vulkan test")
                                .enable_validation_layers()
                                .use_default_debug_messenger()
                                .require_api_version(1, 3, 0)
                                .build();
    if (!vkb_inst_builder) {
        std::println("error: {}", vkb_inst_builder.error().message());
        return -1;
    }

    vkb::Instance vkb_inst = vkb_inst_builder.value();

    VkInstance instance = vkb_inst.instance;
    VkDebugUtilsMessengerEXT debug_messenger = vkb_inst.debug_messenger;
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(win, instance, nullptr, &surface)) {
        std::println("error: {}", SDL_GetError());
        return -1;
    }

    VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    auto physical_device_selected = vkb::PhysicalDeviceSelector{vkb_inst}
                                        .set_minimum_version(1, 3)
                                        .set_required_features_13(features13)
                                        .set_required_features_12(features12)
                                        .set_surface(surface)
                                        .select();
    if (!physical_device_selected) {
        std::println("error: {}", physical_device_selected.error().message());
        return -1;
    }
    auto physical_device = physical_device_selected.value();
    vkb::Device vkb_device = vkb::DeviceBuilder{physical_device}.build().value();

    VkDevice device = vkb_device.device;
    VkPhysicalDevice chosen_gpu = physical_device.physical_device;

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = chosen_gpu;
    allocator_info.device = device;
    allocator_info.instance = instance;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VmaAllocator allocator;
    vmaCreateAllocator(&allocator_info, &allocator);

    auto swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::Swapchain vkb_swapchain = vkb::SwapchainBuilder{chosen_gpu, device, surface}
                                       .set_desired_format(VkSurfaceFormatKHR{
                                           .format = swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                       .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                       .set_desired_extent(1920, 1200)
                                       .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                       .build()
                                       .value();
    VkExtent2D swapchain_extent = vkb_swapchain.extent;
    VkSwapchainKHR swapchain = vkb_swapchain.swapchain;
    std::vector<VkImage> swapchain_images = vkb_swapchain.get_images().value();
    std::vector<VkImageView> swapchain_image_views = vkb_swapchain.get_image_views().value();

    VkQueue graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VkCommandPoolCreateInfo cmd_pool_info{};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = graphics_queue_family;

    FrameData frame_data[FRAMES_IN_FLIGHT];
    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkCreateCommandPool(device, &cmd_pool_info, nullptr, &frame_data[i].cmd_pool);

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.pNext = nullptr;
        cmd_alloc_info.commandPool = frame_data[i].cmd_pool;
        cmd_alloc_info.commandBufferCount = 1;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmd_alloc_info, &frame_data[i].cmd_buf);

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.pNext = nullptr;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &frame_data[i].render_fence));

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;

        VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame_data[i].swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &frame_data[i].render_semaphore));
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForVulkan(win);
    ImGui_ImplVulkan_InitInfo imgui_info{};
    imgui_info.UseDynamicRendering = true;
    imgui_info.Instance = instance;
    imgui_info.PhysicalDevice = chosen_gpu;
    imgui_info.Device = device;
    imgui_info.QueueFamily = graphics_queue_family;
    imgui_info.Queue = graphics_queue;
    // imgui_info.PipelineCache;
    imgui_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
    imgui_info.Subpass = 0;
    imgui_info.MinImageCount = 2;
    imgui_info.ImageCount = 2;
    imgui_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imgui_info.Allocator = allocator->GetAllocationCallbacks();
    imgui_info.CheckVkResultFn = check_vk_result;
    imgui_info.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain_format,
    };
    ImGui_ImplVulkan_Init(&imgui_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    SDL_ShowWindow(win);

    uint64_t accum = 0;
    uint64_t last_time = SDL_GetTicks();

    bool done = false;
    uint8_t i = 0;
    uint64_t frame_count = 0;
    while (!done) {
        uint64_t time = SDL_GetTicks();
        uint64_t frame_time = time - last_time;
        last_time = time;
        accum += frame_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(win))
                done = true;
        }

        if (SDL_GetWindowFlags(win) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        while (accum >= 1000 / 60) {
            accum -= 1000 / 60;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // { ImGui::ShowDemoWindow(); }
        {
            ImGui::Begin("shello");
            ImGui::End();
        }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        VK_CHECK(vkWaitForFences(device, 1, &frame_data[i].render_fence, true, 1000000000));
        VK_CHECK(vkResetFences(device, 1, &frame_data[i].render_fence));

        uint32_t swapchain_image_ix;
        if (vkAcquireNextImageKHR(device, swapchain, 0, frame_data[i].swapchain_semaphore, nullptr,
                                       &swapchain_image_ix) == VK_NOT_READY) {
            continue;
        }

        VkCommandBuffer cmd_buf = frame_data[i].cmd_buf;
        VK_CHECK(vkResetCommandBuffer(cmd_buf, 0));

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.pInheritanceInfo = nullptr;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmd_buf, &begin_info));

        transition_image(cmd_buf, swapchain_images[swapchain_image_ix], VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clear_value;
        float flash = std::abs(std::sin(frame_count / 120.0f));
        *(glm::vec4*)(&clear_value.float32) = {0.0f, 0.0f, flash, 1.0f};

        VkImageSubresourceRange clear_range{};
        clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clear_range.baseMipLevel = 0;
        clear_range.levelCount = VK_REMAINING_MIP_LEVELS;
        clear_range.baseArrayLayer = 0;
        clear_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        vkCmdClearColorImage(cmd_buf, swapchain_images[swapchain_image_ix], VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1,
                             &clear_range);

        if (!(draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)) {
            VkRenderingAttachmentInfo color_attachment{};
            color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            color_attachment.pNext = nullptr;
            color_attachment.imageView = swapchain_image_views[swapchain_image_ix];
            color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            *(glm::vec4*)(&color_attachment.clearValue.color.float32) = {0.0f, 0.0f, 0.0f, 1.0f};

            VkRenderingInfo rendering_info{};
            rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            rendering_info.pNext = nullptr;
            rendering_info.colorAttachmentCount = 1;
            rendering_info.pColorAttachments = &color_attachment;
            rendering_info.layerCount = 1;
            rendering_info.pDepthAttachment = nullptr;
            rendering_info.pStencilAttachment = nullptr;
            rendering_info.renderArea = VkRect2D{.offset = VkOffset2D{}, .extent = swapchain_extent};

            vkCmdBeginRendering(cmd_buf, &rendering_info);

            ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);

            vkCmdEndRendering(cmd_buf);
        }

        transition_image(cmd_buf, swapchain_images[swapchain_image_ix], VK_IMAGE_LAYOUT_GENERAL,
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(cmd_buf));

        VkCommandBufferSubmitInfo cmd_submit_info{};
        cmd_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_submit_info.pNext = nullptr;
        cmd_submit_info.commandBuffer = cmd_buf;
        cmd_submit_info.deviceMask = 0;

        VkSemaphoreSubmitInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        semaphore_info.pNext = nullptr;
        semaphore_info.deviceIndex = 0;
        semaphore_info.value = 1;

        VkSemaphoreSubmitInfo wait_info = semaphore_info;
        wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        wait_info.semaphore = frame_data[i].swapchain_semaphore;

        VkSemaphoreSubmitInfo signal_info = semaphore_info;
        signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        signal_info.semaphore = frame_data[i].render_semaphore;

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreInfoCount = 1;
        submit_info.pWaitSemaphoreInfos = &wait_info;
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = &signal_info;
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &cmd_submit_info;

        VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit_info, frame_data[i].render_fence));

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.pSwapchains = &swapchain;
        present_info.swapchainCount = 1;
        present_info.pWaitSemaphores = &frame_data[i].render_semaphore;
        present_info.waitSemaphoreCount = 1;
        present_info.pImageIndices = &swapchain_image_ix;

        VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));

        i = (i + 1) % FRAMES_IN_FLIGHT;
        frame_count++;
    }

    vkDeviceWaitIdle(device);

    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();

    vmaDestroyAllocator(allocator);

    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(device, frame_data[i].cmd_pool, nullptr);

        vkDestroyFence(device, frame_data[i].render_fence, nullptr);

        vkDestroySemaphore(device, frame_data[i].swapchain_semaphore, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    for (std::size_t i = 0; i < swapchain_image_views.size(); i++) {
        vkDestroyImageView(device, swapchain_image_views[i], nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkb::destroy_debug_utils_messenger(instance, debug_messenger);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(win);
    return 0;
}
