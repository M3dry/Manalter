#define VK_NO_PROTOTYPES 1
#include <cstring>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "vk-bootstrap/src/VkBootstrap.h"
#include <cmath>
#include <csignal>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <span>

#include <print>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <volk.h>

#define VMA_IMPLEMENTATION 1
#include <vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#define VK_CHECK(x)                                                                                                    \
    do {                                                                                                               \
        VkResult err = x;                                                                                              \
        if (err) {                                                                                                     \
            std::println("Detected Vulkan error: {} @{}", string_VkResult(err), __LINE__);                             \
            exit(-1);                                                                                                  \
        }                                                                                                              \
    } while (0)

#define FRAMES_IN_FLIGHT 3

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

namespace shader {
    std::pair<std::unique_ptr<uint8_t[]>, std::size_t> load_shader_code(const char* file_path) {
        std::size_t code_size;
        std::unique_ptr<uint8_t[]> code{(uint8_t*)SDL_LoadFile(file_path, &code_size)};

        return {std::move(code), code_size};
    }

    VkShaderCreateInfoEXT create_info(VkShaderStageFlagBits stage, VkShaderStageFlags next_stage,
                                      std::span<uint8_t> spriv_code, std::span<VkPushConstantRange> push_constants,
                                      std::span<VkDescriptorSetLayout> set_layouts) {
        return VkShaderCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage,
            .nextStage = next_stage,
            .codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
            .codeSize = spriv_code.size(),
            .pCode = spriv_code.data(),
            .pName = "main",
            .setLayoutCount = (uint32_t)set_layouts.size(),
            .pSetLayouts = set_layouts.data(),
            .pushConstantRangeCount = (uint32_t)push_constants.size(),
            .pPushConstantRanges = push_constants.data(),
        };
    }

    VkShaderEXT create(VkDevice device, VkShaderCreateInfoEXT shader) {
        VkShaderEXT shader_ext;
        VK_CHECK(vkCreateShadersEXT(device, 1, &shader, nullptr, &shader_ext));
        return shader_ext;
    }
}

namespace transport {
    uint32_t queue_family;
    VkQueue queue;

    VkCommandPool pool;
    VkCommandBuffer cmd_buf;

    bool coherent = false;
    uint8_t* ring_buffer_data;
    VkBuffer ring_buffer;
    VmaAllocation ring_buffer_allocation;

    uint64_t current_signal = 0;
    VkSemaphore timeline;

    void init(VkDevice device, VkQueue transport_queue, VkCommandPool transport_pool, uint32_t queue_family_ix,
              VmaAllocator alloc) {
        queue_family = queue_family_ix;
        queue = transport_queue;

        pool = transport_pool;

        VkCommandBufferAllocateInfo transport_alloc_info{};
        transport_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        transport_alloc_info.commandBufferCount = 1;
        transport_alloc_info.commandPool = transport_pool;
        transport_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &transport_alloc_info, &cmd_buf);

        VkBufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 1;
        create_info.pQueueFamilyIndices = &queue_family;
        create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.size = 64000000;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo out_alloc_info;
        vmaCreateBuffer(alloc, &create_info, &alloc_info, &ring_buffer, &ring_buffer_allocation, &out_alloc_info);

        ring_buffer_data = (uint8_t*)out_alloc_info.pMappedData;

        VkMemoryPropertyFlags props;
        vmaGetMemoryTypeProperties(alloc, out_alloc_info.memoryType, &props);
        coherent = props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkSemaphoreTypeCreateInfo semaphore_type{};
        semaphore_type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        semaphore_type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        semaphore_type.initialValue = 0;

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = &semaphore_type;

        vkCreateSemaphore(device, &semaphore_info, nullptr, &timeline);
    }

    // `barrer` needs dstStageMask, dstAccessMask and dstQueueFamilyIndex set
    // the QueueFamilyIndexes will be swapped so the barrier can be easily applied in the main queue
    uint64_t upload(VmaAllocator alloc, VkBufferMemoryBarrier2* barrier, void* src, uint32_t size, VkBuffer dst,
                    uint32_t dst_start_offset = 0) {
        std::memcpy(ring_buffer_data, src, size);
        if (!coherent) {
            vmaFlushAllocation(alloc, ring_buffer_allocation, 0, size);
        }

        vkResetCommandBuffer(cmd_buf, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(cmd_buf, &begin_info));

        VkBufferCopy region{};
        region.srcOffset = 0;
        region.size = size;
        region.dstOffset = dst_start_offset;
        vkCmdCopyBuffer(cmd_buf, ring_buffer, dst, 1, &region);

        auto dstStage = barrier->dstStageMask;
        auto dstAccess = barrier->dstAccessMask;
        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier->srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier->srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier->dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier->dstAccessMask = 0;
        barrier->buffer = dst;
        barrier->offset = dst_start_offset;
        barrier->size = size;
        barrier->srcQueueFamilyIndex = queue_family;

        VkDependencyInfo dep_info{};
        dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep_info.bufferMemoryBarrierCount = 1;
        dep_info.pBufferMemoryBarriers = barrier;
        vkCmdPipelineBarrier2(cmd_buf, &dep_info);

        uint32_t dst_queue_family = barrier->dstQueueFamilyIndex;
        barrier->srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier->srcAccessMask = 0;
        barrier->srcAccessMask = dstStage;
        barrier->dstStageMask = dstAccess;
        barrier->dstQueueFamilyIndex = barrier->srcQueueFamilyIndex;
        barrier->srcQueueFamilyIndex = dst_queue_family;

        VK_CHECK(vkEndCommandBuffer(cmd_buf));

        VkCommandBufferSubmitInfo cmd_info{};
        cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cmd_info.commandBuffer = cmd_buf;

        VkSemaphoreSubmitInfo signal_info{};
        signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal_info.semaphore = timeline;
        signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        signal_info.value = ++current_signal;

        VkSubmitInfo2 submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.waitSemaphoreInfoCount = 0;
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &cmd_info;
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = &signal_info;

        VK_CHECK(vkQueueSubmit2(queue, 1, &submit_info, nullptr));

        return current_signal;
    }

    void deinit(VkDevice device, VmaAllocator alloc) {
        vmaDestroyBuffer(alloc, ring_buffer, ring_buffer_allocation);
        vkDestroySemaphore(device, timeline, nullptr);
    }
}

int main(int argc, char** argv) {
    VK_CHECK(volkInitialize());

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
    volkLoadInstance(instance);

    VkDebugUtilsMessengerEXT debug_messenger = vkb_inst.debug_messenger;
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(win, instance, nullptr, &surface)) {
        std::println("error: {}", SDL_GetError());
        return -1;
    }

    VkPhysicalDeviceVulkan14Features features14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};
    features14.maintenance5 = true;
    VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.timelineSemaphore = true;

    auto physical_device_selected = vkb::PhysicalDeviceSelector{vkb_inst}
                                        .set_minimum_version(1, 4)
                                        .set_required_features_13(features13)
                                        .set_required_features_12(features12)
                                        .set_surface(surface)
                                        .add_required_extensions({"VK_EXT_shader_object"})
                                        .select();
    if (!physical_device_selected) {
        std::println("error: {}", physical_device_selected.error().message());
        return -1;
    }

    auto physical_device = physical_device_selected.value();

    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> physical_device_extensions{extension_count};
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, physical_device_extensions.data());

    bool latest_ready_extension_flag = std::any_of(
        physical_device_extensions.begin(), physical_device_extensions.end(), [](const VkExtensionProperties& ext) {
            return std::strcmp(ext.extensionName, "VK_EXT_present_mode_fifo_latest_ready") == 0;
        });

    VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT latest_ready_extension{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_EXT,
        .pNext = nullptr,
        .presentModeFifoLatestReady = true,
    };
    VkPhysicalDeviceShaderObjectFeaturesEXT shader_object_extension{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .pNext = latest_ready_extension_flag ? &latest_ready_extension : nullptr,
        .shaderObject = true,
    };
    vkb::Device vkb_device = vkb::DeviceBuilder{physical_device}.add_pNext(&shader_object_extension).build().value();

    VkDevice device = vkb_device.device;
    VkPhysicalDevice chosen_gpu = physical_device.physical_device;

    volkLoadDevice(device);
    vkCmdPushConstants2 = (PFN_vkCmdPushConstants2) vkGetInstanceProcAddr(instance, "vkCmdPushConstants2");

    VmaVulkanFunctions allocator_funcs{};
    allocator_funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    allocator_funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = chosen_gpu;
    allocator_info.device = device;
    allocator_info.instance = instance;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocator_info.pVulkanFunctions = &allocator_funcs;
    VmaAllocator allocator;
    vmaCreateAllocator(&allocator_info, &allocator);

    auto swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::Swapchain vkb_swapchain =
        vkb::SwapchainBuilder{chosen_gpu, device, surface}
            .set_desired_format(
                VkSurfaceFormatKHR{.format = swapchain_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(latest_ready_extension_flag ? VK_PRESENT_MODE_FIFO_LATEST_READY_EXT
                                                                  : VK_PRESENT_MODE_IMMEDIATE_KHR)
            .set_desired_extent(1920, 1200)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_desired_min_image_count(2)
            .build()
            .value();
    VkExtent2D swapchain_extent = vkb_swapchain.extent;
    VkSwapchainKHR swapchain = vkb_swapchain.swapchain;
    std::vector<VkImage> swapchain_images = vkb_swapchain.get_images().value();
    std::vector<VkImageView> swapchain_image_views = vkb_swapchain.get_image_views().value();

    VkQueue graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VkQueue transport_queue = vkb_device.get_queue(vkb::QueueType::transfer).value();
    auto transport_queue_family = vkb_device.get_queue_index(vkb::QueueType::transfer).value();

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

    VkCommandPoolCreateInfo transport_pool_info{};
    transport_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    transport_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    transport_pool_info.queueFamilyIndex = transport_queue_family;

    VkCommandPool transport_pool{};
    vkCreateCommandPool(device, &transport_pool_info, nullptr, &transport_pool);

    transport::init(device, transport_queue, transport_pool, transport_queue_family, allocator);

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
    imgui_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
    imgui_info.Subpass = 0;
    imgui_info.MinImageCount = 2;
    imgui_info.ImageCount = 2;
    imgui_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imgui_info.Allocator = nullptr;
    imgui_info.CheckVkResultFn = check_vk_result;
    imgui_info.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain_format,
    };
    ImGui_ImplVulkan_Init(&imgui_info);

    auto [vertex_code, vertex_size] = shader::load_shader_code("./vertex.spv");
    auto [fragment_code, fragment_size] = shader::load_shader_code("./fragment.spv");
    VkPushConstantRange vertex_push_constant{};
    vertex_push_constant.size = sizeof(uint64_t);
    vertex_push_constant.offset = 0;
    vertex_push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    auto vertex_info =
        shader::create_info(VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, {vertex_code.get(), vertex_size},
                            {&vertex_push_constant, 1}, {(VkDescriptorSetLayout*)nullptr, 0});
    auto fragment_info = shader::create_info(VK_SHADER_STAGE_FRAGMENT_BIT, 0, {fragment_code.get(), fragment_size},
                                             {(VkPushConstantRange*)nullptr, 0}, {(VkDescriptorSetLayout*)nullptr, 0});
    auto vertex = shader::create(device, vertex_info);
    auto fragment = shader::create(device, fragment_info);

    SDL_ShowWindow(win);

    VkBuffer vertices;
    VmaAllocation vertices_allocation;
    glm::vec4 vertices_data[3] = {
        glm::vec4{0.0, 0.5, 0.0f, 1.0f},
        glm::vec4{-0.5, -0.5, 0.0f, 1.0f},
        glm::vec4{0.5, -0.5, 0.0f, 1.0f},
    };

    {
        VkBufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.size = sizeof(glm::vec4) * 3;
        create_info.queueFamilyIndexCount = 1;
        create_info.pQueueFamilyIndices = &graphics_queue_family;
        create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        vmaCreateBuffer(allocator, &create_info, &alloc_info, &vertices, &vertices_allocation, nullptr);
    }

    VkBufferMemoryBarrier2 vertices_barrier;
    vertices_barrier.dstQueueFamilyIndex = graphics_queue_family;
    vertices_barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    vertices_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

    auto vertices_time = transport::upload(allocator, &vertices_barrier, vertices_data, sizeof(glm::vec4) * 3, vertices, 0);

    uint64_t accum = 0;
    uint64_t last_time = SDL_GetTicks();

    bool done = false;
    uint64_t frame_count = 0;
    while (!done) {
        vkDeviceWaitIdle(device); // gets rid of the fucking validation error

        uint64_t time = SDL_GetTicks();
        uint64_t frame_time = time - last_time;
        last_time = time;
        accum += frame_time;

        FrameData& frame = frame_data[frame_count % 2];
        VK_CHECK(vkWaitForFences(device, 1, &frame.render_fence, true, UINT64_MAX));
        VK_CHECK(vkResetFences(device, 1, &frame.render_fence));

        uint32_t image_ix;
        auto result =
            vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.swapchain_semaphore, VK_NULL_HANDLE, &image_ix);
        if (result != VK_SUCCESS) std::println("acquire result: {}", string_VkResult(result));

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

        { ImGui::ShowDemoWindow(); }

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        vkResetCommandBuffer(frame.cmd_buf, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(frame.cmd_buf, &begin_info));

        transition_image(frame.cmd_buf, swapchain_images[image_ix], VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        if (!(draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)) {
            float flash = std::abs(std::sin(frame_count / 1200.0f));

            VkRenderingAttachmentInfo color_attachment{};
            color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            color_attachment.pNext = nullptr;
            color_attachment.imageView = swapchain_image_views[image_ix];
            color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

            vkCmdBeginRendering(frame.cmd_buf, &rendering_info);

            vkCmdSetCullModeEXT(frame.cmd_buf, VK_CULL_MODE_NONE);
            vkCmdSetDepthTestEnableEXT(frame.cmd_buf, false);
            vkCmdSetDepthWriteEnableEXT(frame.cmd_buf, false);
            vkCmdSetRasterizerDiscardEnableEXT(frame.cmd_buf, false);
            vkCmdSetPrimitiveRestartEnableEXT(frame.cmd_buf, false);
            vkCmdSetStencilTestEnableEXT(frame.cmd_buf, false);
            vkCmdSetDepthBiasEnableEXT(frame.cmd_buf, false);
            vkCmdSetPolygonModeEXT(frame.cmd_buf, VK_POLYGON_MODE_FILL);
            vkCmdSetRasterizationSamplesEXT(frame.cmd_buf, VK_SAMPLE_COUNT_1_BIT);
            vkCmdSetVertexInputEXT(frame.cmd_buf, 0, nullptr, 0, nullptr);
            vkCmdSetPrimitiveTopologyEXT(frame.cmd_buf, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

            uint32_t enable_blend = false;
            vkCmdSetColorBlendEnableEXT(frame.cmd_buf, 0, 1, &enable_blend);
            VkSampleMask sample_mask = 0xFFFFFFFF;
            vkCmdSetSampleMaskEXT(frame.cmd_buf, VK_SAMPLE_COUNT_1_BIT, &sample_mask);
            vkCmdSetFrontFaceEXT(frame.cmd_buf, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            vkCmdSetAlphaToCoverageEnableEXT(frame.cmd_buf, false);
            VkViewport viewport{
                .x = 0,
                .y = 1200,
                .width = 1920,
                .height = -1200,
                .minDepth = 0,
                .maxDepth = 1,
            };
            vkCmdSetViewportWithCountEXT(frame.cmd_buf, 1, &viewport);
            VkRect2D scissor{
                .offset =
                    VkOffset2D{
                        .x = 0,
                        .y = 0,
                    },
                .extent =
                    VkExtent2D{
                        .width = 1920,
                        .height = 1200,
                    },
            };
            vkCmdSetScissorWithCountEXT(frame.cmd_buf, 1, &scissor);
            VkColorComponentFlags color_components{};
            color_components |= VK_COLOR_COMPONENT_R_BIT;
            color_components |= VK_COLOR_COMPONENT_G_BIT;
            color_components |= VK_COLOR_COMPONENT_B_BIT;
            color_components |= VK_COLOR_COMPONENT_A_BIT;
            vkCmdSetColorWriteMaskEXT(frame.cmd_buf, 0, 1, &color_components);

            VkBufferDeviceAddressInfo vertices_address_info{};
            vertices_address_info.buffer = vertices;
            vertices_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

            uint64_t vertices_address = vkGetBufferDeviceAddress(device, &vertices_address_info);
            VkPushConstantsInfo push_constant_info{};
            push_constant_info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
            push_constant_info.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            push_constant_info.offset = 0;
            push_constant_info.pValues = &vertices_address;
            push_constant_info.size = sizeof(uint64_t);
            assert(vkCmdPushConstants2 != nullptr);
            vkCmdPushConstants2(frame.cmd_buf, &push_constant_info);

            VkShaderStageFlagBits stages[2] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
            VkShaderEXT shaders[2] = {vertex, fragment};
            vkCmdBindShadersEXT(frame.cmd_buf, 2, stages, shaders);
            vkCmdDraw(frame.cmd_buf, 3, 1, 0, 0);

            ImGui_ImplVulkan_RenderDrawData(draw_data, frame.cmd_buf);

            vkCmdEndRendering(frame.cmd_buf);
        }

        transition_image(frame.cmd_buf, swapchain_images[image_ix], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
        present_info.pImageIndices = &image_ix;

        result = vkQueuePresentKHR(graphics_queue, &present_info);
        if (result != VK_SUCCESS) std::println("present result: {}", string_VkResult(result));

        frame_count++;
    }

    vkDeviceWaitIdle(device);

    vkDestroyShaderEXT(device, vertex, nullptr);
    vkDestroyShaderEXT(device, fragment, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    transport::deinit(device, allocator);

    vmaDestroyBuffer(allocator, vertices, vertices_allocation);

    vmaDestroyAllocator(allocator);

    vkDestroyCommandPool(device, transport_pool, nullptr);

    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(device, frame_data[i].cmd_pool, nullptr);

        vkDestroyFence(device, frame_data[i].render_fence, nullptr);

        vkDestroySemaphore(device, frame_data[i].swapchain_semaphore, nullptr);
        vkDestroySemaphore(device, frame_data[i].render_semaphore, nullptr);
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
