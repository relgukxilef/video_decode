#pragma once

#include <stdexcept>

#include <vulkan/vulkan.h>

#include "resource.h"

extern VkInstance current_instance;
extern VkDevice current_device;

struct vulkan_out_of_memory_error : public std::bad_alloc {
    vulkan_out_of_memory_error() : std::bad_alloc() {}
};

struct vulkan_device_lost : public std::runtime_error {
    vulkan_device_lost() : std::runtime_error("Device lost") {}
};

inline void check(VkResult success) {
    if (success == VK_SUCCESS) {
        return;
    } else if (success == VK_ERROR_OUT_OF_HOST_MEMORY) {
        throw vulkan_out_of_memory_error();
    } else if (success == VK_ERROR_DEVICE_LOST) {
        throw vulkan_device_lost();
    } else {
        throw std::runtime_error("Unknown vulkan error");
    }
}

template<typename T, auto Deleter>
void vulkan_delete(T* t) {
    Deleter(current_device, *t, nullptr);
}

inline void vulkan_delete_instance(VkInstance* device) {
    vkDestroyInstance(*device, nullptr);
}

inline void vulkan_delete_debug_utils_messenger(
    VkDebugUtilsMessengerEXT* messenger
) {
    // TODO: is there a better place to load the function?
    auto vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            current_instance, "vkDestroyDebugUtilsMessengerEXT"
    );
    vkDestroyDebugUtilsMessengerEXT(current_instance, *messenger, nullptr);
}

inline void vulkan_delete_device(VkDevice* device) {
    vkDestroyDevice(*device, nullptr);
}

inline void vulkan_delete_surface(VkSurfaceKHR* surface) {
    vkDestroySurfaceKHR(current_instance, *surface, nullptr);
}

template<typename T, auto Deleter>
using unique_vulkan_resource =
    unique_resource<T, vulkan_delete<T, Deleter>, VK_NULL_HANDLE>;


typedef unique_resource<
    VkDebugUtilsMessengerEXT, vulkan_delete_debug_utils_messenger,
    VK_NULL_HANDLE
> unique_debug_utils_messenger;

typedef unique_resource<VkInstance, vulkan_delete_instance, VK_NULL_HANDLE>
    unique_instance;

typedef unique_resource<VkDevice, vulkan_delete_device, VK_NULL_HANDLE>
    unique_device;

typedef unique_resource<VkSurfaceKHR, vulkan_delete_surface, VK_NULL_HANDLE>
    unique_surface;

typedef unique_vulkan_resource<VkShaderModule, vkDestroyShaderModule>
    unique_shader_module;

typedef unique_vulkan_resource<VkFramebuffer, vkDestroyFramebuffer>
    unique_framebuffer;

typedef unique_vulkan_resource<VkPipeline, vkDestroyPipeline>
    unique_pipeline;

typedef unique_vulkan_resource<VkImage, vkDestroyImage> unique_image;

typedef unique_vulkan_resource<VkImageView, vkDestroyImageView>
    unique_image_view;

typedef unique_vulkan_resource<VkPipelineLayout, vkDestroyPipelineLayout>
    unique_pipeline_layout;

typedef unique_vulkan_resource<
    VkDescriptorSetLayout, vkDestroyDescriptorSetLayout
> unique_descriptor_set_layout;

typedef unique_vulkan_resource<VkRenderPass, vkDestroyRenderPass>
    unique_render_pass;

typedef unique_vulkan_resource<VkCommandPool, vkDestroyCommandPool>
    unique_command_pool;

typedef unique_vulkan_resource<VkFence, vkDestroyFence> unique_fence;

typedef unique_vulkan_resource<VkSemaphore, vkDestroySemaphore>
    unique_semaphore;

typedef unique_vulkan_resource<VkSwapchainKHR, vkDestroySwapchainKHR>
    unique_swapchain;

typedef unique_vulkan_resource<VkBuffer, vkDestroyBuffer>
    unique_buffer;

typedef unique_vulkan_resource<VkDeviceMemory, vkFreeMemory>
    unique_device_memory;

typedef unique_vulkan_resource<VkDescriptorPool, vkDestroyDescriptorPool>
    unique_descriptor_pool;
