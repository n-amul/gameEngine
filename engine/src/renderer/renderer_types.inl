#pragma once

#include "defines.h"

typedef enum renderer_backend_type{
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX
}renderer_backend_type;

typedef struct global_uniform_object {
    mat4 projection;   // 64 bytes
    mat4 view;         // 64 bytes
    mat4 m_reserved0;  // 64 bytes, reserved for future use
    mat4 m_reserved1;  // 64 bytes, reserved for future use
} global_uniform_object;

typedef struct renderer_backend{
    u64 frame_number;

    b8 (*initialize)(struct renderer_backend* backend,const char* applicatation_name);
    void (*shutdown)(struct renderer_backend* backend);
    void (*resized)(struct renderer_backend* backend, u16 width, u16 height);

    b8 (*begin_frame)(struct renderer_backend* backend, f32 delta_time);//Prepares Vulkan each frame before drawing starts.
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);//Wraps up drawing and presents the final image to the screen.
}renderer_backend;

typedef struct render_packet {
    f32 delta_time;
}render_packet;