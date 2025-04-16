#pragma once

#include "defines.h"

struct game;

//application config
typedef struct application_config{
    //window staring pos
    i16 start_pos_x;
    i16 start_pos_y;

    //window starting width height
    i16 start_width;
    i16 start_height;
    //application name used in windowing
    char* name;
}application_config;

KAPI b8 application_create(struct game* game_inst);

KAPI b8 application_run();

void application_get_framebuffer_size(u32* width, u32* height);
