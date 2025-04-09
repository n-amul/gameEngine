#pragma once

#include "vulkan_types.inl"

void vulkan_swapchain_create(
    vulkan_context* context,
    u32 width,
    u32 height,
    vulkan_swapchain* out_swapchain);

// once swapchain is created its immutable (e.g. resize)
void vulkan_swapchain_recreate(
    vulkan_context* context,
    u32 width,
    u32 height,
    vulkan_swapchain* out_swapchain);

void vulkan_swapchain_destroy(
    vulkan_context* context,
    vulkan_swapchain* swapchain);

//gives next index of image to be used.
b8 vulkan_swapchain_acquire_next_image_index(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    u64 timeout_ns,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* out_image_index);

//displays (presents) the image you just finished rendering onto the screen.
void vulkan_swapchain_present(
    vulkan_context* context,
    vulkan_swapchain* swapchain,
    VkQueue graphics_queue,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 present_image_index);