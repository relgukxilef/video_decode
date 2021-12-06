#pragma once

#include <memory>

#include "../utility/vulkan_resource.h"

struct ui {
    ui(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    unique_device device;
    unique_command_pool command_pool;
};

