#pragma once

#include "defines.h"

b8 platform_system_startup(
    u64* memory_requirement,
    void* state,
    const char* application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height);

void platform_system_shutdown(void* plat_state);

b8 platform_pump_messages();

void* platform_allocate(u64 size,b8 aligned);
void platform_free(void* block,b8 aligned);
void* platform_zero_memory(void* block,u64 size);
void* platform_copy_memory(void* dest,const void* src,u64 size);
void* platform_set_memory(void* dest,i32 val,u64 size);

void platform_console_write(const char* message,u8 color);
void platform_console_write_error(const char* message,u8 color);

f64 platform_get_absolute_time();

//sleep on the thread for ms, this blocks the main thread.
//only be used for giving time back to the IS for unused update power so not exported.
void platform_sleep(u64 ms);

