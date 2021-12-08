#include "ui.h"

#include "../utility/out_ptr.h"

extern uint32_t _binary_ui_video_vertex_glsl_spv_start;
extern uint32_t _binary_ui_video_vertex_glsl_spv_end;
extern uint32_t _binary_ui_video_fragment_glsl_spv_start;
extern uint32_t _binary_ui_video_fragment_glsl_spv_end;

void create_shader(
    unique_device& device, uint32_t& begin, uint32_t& end,
    unique_shader_module& module
) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>((&end - &begin) * 4),
        .pCode = &begin,
    };

    check(vkCreateShaderModule(
        device.get(), &create_info, nullptr, out_ptr(module)
    ));
}

image::image(ui& ui, view& view, VkImage image) {
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    check(vkCreateSemaphore(
        ui.device.get(), &semaphore_info, nullptr,
        out_ptr(render_finished_semaphore)
    ));

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    check(vkCreateFence(
        ui.device.get(), &fence_info, nullptr, out_ptr(render_finished_fence)
    ));

    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ui.command_pool.get(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    check(vkAllocateCommandBuffers(
        ui.device.get(), &command_buffer_info, &video_draw_command_buffer
    ));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    check(vkBeginCommandBuffer(video_draw_command_buffer, &begin_info));

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vkCmdPipelineBarrier(
        video_draw_command_buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr, 0, nullptr, 1, &barrier
    );

    check(vkEndCommandBuffer(video_draw_command_buffer));
}

view::view(ui& ui) {
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        ui.physical_device, ui.surface, &capabilities
    ));

    unsigned width = capabilities.currentExtent.width;
    unsigned height = capabilities.currentExtent.height;

    extent = {
        std::max(
            std::min<uint32_t>(width, capabilities.maxImageExtent.width),
            capabilities.minImageExtent.width
        ),
        std::max(
            std::min<uint32_t>(height, capabilities.maxImageExtent.height),
            capabilities.minImageExtent.height
        )
    };

    {
        uint32_t queue_family_indices[]{
            ui.graphics_queue_family, ui.present_queue_family
        };
        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = ui.surface,
            .minImageCount = capabilities.minImageCount,
            .imageFormat = ui.surface_format.format,
            .imageColorSpace = ui.surface_format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = std::size(queue_family_indices),
            .pQueueFamilyIndices = queue_family_indices,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            // fifo has the widest support
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
        check(vkCreateSwapchainKHR(
            ui.device.get(), &create_info, nullptr, out_ptr(swapchain)
        ));
    }

    check(vkGetSwapchainImagesKHR(
        ui.device.get(), swapchain.get(), &image_count, nullptr
    ));

    auto swapchain_images = std::make_unique<VkImage[]>(image_count);
    images = std::make_unique<image[]>(image_count);

    check(vkGetSwapchainImagesKHR(
        ui.device.get(), swapchain.get(), &image_count, swapchain_images.get()
    ));

    for (auto image = 0u; image < image_count; image++) {
        images[image] = ::image(ui, *this, swapchain_images[image]);
    }
}

void view::render(ui& ui) {
    uint32_t image_index;
    check(vkAcquireNextImageKHR(
        ui.device.get(), swapchain.get(), ~0ul,
        ui.swapchain_image_ready_semaphore.get(),
        VK_NULL_HANDLE, &image_index
    ));

    check(vkWaitForFences(
        ui.device.get(), 1, &images[image_index].render_finished_fence.get(),
        VK_TRUE, ~0ul
    ));
    check(vkResetFences(
        ui.device.get(), 1, &images[image_index].render_finished_fence.get()
    ));

    // TODO: maybe move this to image::render
    VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ui.swapchain_image_ready_semaphore.get(),
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &images[image_index].video_draw_command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores =
            &images[image_index].render_finished_semaphore.get(),
    };
    check(vkQueueSubmit(
        ui.graphics_queue, 1, &submitInfo,
        images[image_index].render_finished_fence.get()
    ));

    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &images[image_index].render_finished_semaphore.get(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain.get(),
        .pImageIndices = &image_index,
    };
    check(vkQueuePresentKHR(ui.present_queue, &present_info));
}

ui::ui(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface
) : physical_device(physical_device), surface(surface) {
    // look for available queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, nullptr
    );
    auto queue_families =
        std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, queue_families.get()
    );

    for (auto i = 0u; i < queue_family_count; i++) {
        const auto& queueFamily = queue_families[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device, i, surface, &present_support
        );
        if (present_support) {
            present_queue_family = i;
        }
    }
    if (graphics_queue_family == -1u) {
        throw std::runtime_error("no suitable queue found");
    }


    // create logical device
    {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_infos[]{
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = graphics_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }, {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = present_queue_family,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            }
        };

        const char* enabled_extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        VkPhysicalDeviceFeatures device_features{};
        VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = std::size(queue_create_infos),
            .pQueueCreateInfos = queue_create_infos,
            .enabledExtensionCount = std::size(enabled_extension_names),
            .ppEnabledExtensionNames = enabled_extension_names,
            .pEnabledFeatures = &device_features
        };

        check(vkCreateDevice(
            physical_device, &create_info, nullptr, out_ptr(device)
        ));
    }
    current_device = device.get();

    // retreive queues
    vkGetDeviceQueue(device.get(), graphics_queue_family, 0, &graphics_queue);
    vkGetDeviceQueue(device.get(), present_queue_family, 0, &present_queue);

    // create swap chains
    uint32_t format_count = 0, present_mode_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &format_count, nullptr
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &present_mode_count, nullptr
    );
    if (format_count == 0) {
        throw std::runtime_error("no surface formats supported");
    }
    if (present_mode_count == 0) {
        throw std::runtime_error("no surface present modes supported");
    }
    auto formats = std::make_unique<VkSurfaceFormatKHR[]>(format_count);
    auto present_modes =
        std::make_unique<VkPresentModeKHR[]>(present_mode_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device, surface, &format_count, formats.get()
    );
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &present_mode_count, present_modes.get()
    );

    surface_format = formats[0];
    for (auto i = 0u; i < format_count; i++) {
        auto format = formats[i];
        if (
            format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            surface_format = format;
        }
    }

    {
        VkCommandPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphics_queue_family,
        };
        check(vkCreateCommandPool(
            device.get(), &create_info, nullptr, out_ptr(command_pool)
        ));
    }

    create_shader(
        device, _binary_ui_video_vertex_glsl_spv_start,
        _binary_ui_video_vertex_glsl_spv_end, video_vertex
    );
    create_shader(
        device, _binary_ui_video_fragment_glsl_spv_start,
        _binary_ui_video_fragment_glsl_spv_end, video_fragment
    );

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(
        physical_device, &memory_properties
    );

    {
        unsigned width = 1024, height = 1024;
        VkImageCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8_SRGB,
            .extent = {
                .width = width,
                .height = height,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage =
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        check(vkCreateImage(
            device.get(), &create_info, nullptr, out_ptr(video_image)
        ));
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(
            device.get(), video_image.get(), &memory_requirements
        );

        uint32_t memory_type = 0;
        VkMemoryPropertyFlags properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
            if (
                (memory_requirements.memoryTypeBits & (1 << i)) &&
                (memory_properties.memoryTypes[i].propertyFlags & properties) ==
                properties
            ) {
                memory_type = i;
                break;
            }
        }

        VkMemoryAllocateInfo allocate_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = memory_type,
        };
        check(vkAllocateMemory(
            device.get(), &allocate_info, nullptr, out_ptr(streaming_memory)
        ));

        check(vkBindImageMemory(
            device.get(), video_image.get(), streaming_memory.get(), 0
        ));

        check(vkMapMemory(
            device.get(), streaming_memory.get(), 0, width * height, 0,
            reinterpret_cast<void**>(&video_buffer)
        ));
    }

    {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = video_image.get(),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8_SRGB,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        check(vkCreateImageView(
            device.get(), &create_info, nullptr, out_ptr(video_image_view)
        ));
    }

    {
        VkSemaphoreCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        check(vkCreateSemaphore(
            device.get(), &create_info, nullptr,
            out_ptr(swapchain_image_ready_semaphore)
        ));
    }

    view = ::view(*this);
}

void ui::push_frame(const frame &f) {
    std::move(
        f.pixels.y.get(), f.pixels.y.get() + f.width * f.height, video_buffer
    );
}

void ui::render() {
    try {
        view.render(*this);

    } catch (vulkan_stale_swapchain) {
        view = {}; // delete first
        view = ::view(*this);

        view.render(*this);
    }
}
