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
    unique_device_memory streaming_memory;

    unique_shader_module video_vertex, video_fragment;
    unique_pipeline_layout video_pipeline_layout;
    unique_pipeline video_pipeline;

    unique_image video_image;
    unique_image_view video_image_view;

    uint8_t* video_buffer;

    unique_semaphore swapchain_image_ready_semaphore;

    view view;

    uint32_t graphics_queue_family = -1u, present_queue_family = -1u;
    VkSurfaceFormatKHR surface_format;
};

