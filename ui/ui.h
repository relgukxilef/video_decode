#pragma once

#include <memory>

#include "../utility/vulkan_resource.h"
#include "../data/frame.h"

struct image;
struct view;
struct ui;

struct image {
    image() = default;
    image(ui& ui, view& view, VkImage image);

    unique_framebuffer swapchain_framebuffer;
    unique_image_view swapchain_image_view;

    unique_semaphore render_finished_semaphore;
    unique_fence render_finished_fence;

    VkCommandBuffer video_draw_command_buffer;
};

struct view {
    view() = default;
    view(ui& ui);

    VkResult render(ui &ui);

    unsigned image_count;
    VkSurfaceCapabilitiesKHR capabilities;
    VkExtent2D extent;
    VkViewport viewport;
    VkRect2D scissors;
    unique_swapchain swapchain;

    std::unique_ptr<image[]> images;
};

struct dynamic_image {
    dynamic_image() = default;
    dynamic_image(ui& ui, unsigned size);

    unique_device_memory device_memory;
    unique_image image;
    unique_image_view image_view;
    uint8_t* buffer;
};

struct ui {
    ui() = default;
    ui(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    void push_frame(const frame& f);
    void render();

    VkPhysicalDevice physical_device;
    VkSurfaceKHR surface;

    unique_device device;
    VkQueue graphics_queue, present_queue;
    unique_command_pool command_pool;

    dynamic_image video_y;
    dynamic_image video_cb;
    dynamic_image video_cr;

    unique_sampler video_sampler;

    unique_descriptor_set_layout descriptor_set_layout;
    unique_descriptor_pool descriptor_pool;
    VkDescriptorSet descriptor_set;

    unique_render_pass render_pass;

    unique_pipeline_layout video_pipeline_layout;
    unique_pipeline video_pipeline;

    unique_semaphore swapchain_image_ready_semaphore;

    view view;

    uint32_t graphics_queue_family = -1u, present_queue_family = -1u;
    VkSurfaceFormatKHR surface_format;
    VkPhysicalDeviceMemoryProperties memory_properties;
};

