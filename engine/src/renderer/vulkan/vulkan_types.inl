#pragma once
 
#include "defines.h"
#include "core/asserts.h"
 
#include <vulkan/vulkan.h>

//checks the givien expression's return value against VK_SUCESS.
#define VK_CHECK(expr)        \
{                             \
    KASSERT(expr==VK_SUCCESS);\
}
 
typedef struct vulkan_swapchain_support_info {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 format_count;
    VkSurfaceFormatKHR* formats;
    u32 present_mode_count;
    VkPresentModeKHR* present_modes;
} vulkan_swapchain_support_info;

typedef struct vulkan_device {
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    vulkan_swapchain_support_info swapchain_support;
    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkPhysicalDeviceProperties properties; //General information about the GPU, such as its name, driver version, and supported features.
    VkPhysicalDeviceFeatures features; //Specific hardware capabilities that the GPU supports (for example, geometry shaders or tessellation).
    VkPhysicalDeviceMemoryProperties memory; //Details about the types of memory the GPU has and how to access them.

    VkCommandPool graphics_command_pool;

    VkFormat depth_format;
} vulkan_device;

typedef struct vulkan_image {
    VkImage handle;          // The actual image object used by Vulkan (like a GPU texture).
    VkDeviceMemory memory;   // Memory allocated for this image.
    VkImageView view;        // A "view" that describes how to access the image (e.g., as a 2D texture).
    u32 width;               // Width of the image.
    u32 height;              // Height of the image.
} vulkan_image;

typedef enum vulkan_render_pass_state{
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
}vulkan_render_pass_state;

typedef struct vulkan_renderpass{
    VkRenderPass handle;
    f32 x,y,w,h;
    f32 r,g,b,a;
    f32 depth;
    u32 stencil;
    vulkan_render_pass_state state;
}vulkan_renderpass;

typedef struct vulkan_framebuffer {
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView* attachments;
    vulkan_renderpass* renderpass;
} vulkan_framebuffer;

typedef struct vulkan_swapchain {
    VkSurfaceFormatKHR image_format;    // Format and color space of the images for displaying.
    u8 max_frames_in_flight;            // How many frames can be processed at once.
    VkSwapchainKHR handle;              // The actual Vulkan swapchain object.
    u32 image_count;                    // Number of images in the swapchain.
    VkImage* images;                    // Array of image handles.
    VkImageView* views;                 // Array of views for those images. we access image using this.
    vulkan_framebuffer* framebuffers;   // fb for onscreen rendering
    vulkan_image depth_attachment;      // An image used for depth buffering (helps with 3D depth).
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

typedef struct vulkan_command_buffer {
    VkCommandBuffer handle;

    // Command buffer state.
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_fence {
    VkFence handle;
    b8 is_signaled;
} vulkan_fence;

typedef struct vulkan_context {
    //framebuffer current width height
    u32 framebuffer_width;
    u32 framebuffer_height;

    //current generation of framebuffer size
    //if last one doesnt match, new one should be generated
    u64 framebuffer_size_generation;
    u64 framebuffer_size_last_generation;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
    //vulkan_device device;
    vulkan_device device;
    vulkan_swapchain swapchain;
    vulkan_renderpass main_renderpass;

    // darray
    vulkan_command_buffer* graphics_command_buffers;
    // darray 
    //  tells the presentation queue an image from the swapchain is ready to be drawn to
    VkSemaphore* image_available_semaphores;
    // darray
    //Signal when rendering is done
    VkSemaphore* queue_complete_semaphores;

    u32 in_flight_fence_count;
    vulkan_fence* in_flight_fences;	//CPU waits until GPU finish its work (CPU ↔ GPU sync)
    // Holds pointers to fences which exist and are owned elsewhere.
    vulkan_fence** images_in_flight; 
    //Tracks which fence is using which swapchain image (A checklist that says, "Is this image still busy? If yes, wait.")

    u32 image_index;
    u32 current_frame;

    b8 recreating_swapchain;
    i32 (*find_memory_index)(u32 type_filter,u32 property_flags);
}vulkan_context;

