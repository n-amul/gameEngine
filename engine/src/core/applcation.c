#include "application.h"

#include "platform/platform.h"
#include "game_types.h"

#include "logger.h"

#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"
#include "renderer/renderer_frontend.h"

#include "memory/linear_allocator.h"

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    i16 width;
    i16 height;
    f64 last_time;
    clock clock;
    linear_allocator systems_allocator;

    u64 event_system_memory_requirement;
    void* event_system_state;

    u64 memory_system_memory_requirement;
    void* memory_system_state;
    
    u64 logging_system_memory_requirement;
    void* logging_system_state;

    u64 input_system_memory_requirement;
    void* input_system_state;
 
    u64 platform_system_memory_requirement;
    void* platform_system_state;
 
    u64 renderer_system_memory_requirement;
    void* renderer_system_state;
} application_state;

static application_state* app_state;

//event handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_resize(u16 code, void* sender, void* listener_inst, event_context context);

b8 application_create(game* game_inst){
    if(game_inst->application_state){
        KERROR("application is already created.");
        return false;
    }
    game_inst->application_state = kallocate(sizeof(application_state),MEMORY_TAG_APPLICATION);
    app_state = game_inst->application_state;
    app_state->game_inst = game_inst;

    //initialize subsystems.

    u64 systems_allocator_total_size = 64*1024*1024; //64mb
    linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);
    //Memory
    memory_system_initialize(&app_state->memory_system_memory_requirement, 0);
    app_state->memory_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->memory_system_memory_requirement);
    memory_system_initialize(&app_state->memory_system_memory_requirement, app_state->memory_system_state);

    //Logger
    initialize_logging(&app_state->logging_system_memory_requirement,0);
    app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
    if(!initialize_logging(&app_state->logging_system_memory_requirement,app_state->logging_system_state)){
        KERROR("failed to init logging system. shutting down.");
        return false;
    }
    // Input
    input_system_initialize(&app_state->input_system_memory_requirement, 0);
    app_state->input_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->input_system_memory_requirement);
    input_system_initialize(&app_state->input_system_memory_requirement, app_state->input_system_state);

    //Events
    event_system_initialize(&app_state->event_system_memory_requirement, 0);
    app_state->event_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->event_system_memory_requirement);
    event_system_initialize(&app_state->event_system_memory_requirement, app_state->event_system_state);

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    event_register(EVENT_CODE_RESIZED, 0, application_on_resize);

    // Platform
    platform_system_startup(&app_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
    app_state->platform_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->platform_system_memory_requirement);
    if (!platform_system_startup(
        &app_state->platform_system_memory_requirement,
        app_state->platform_system_state,
        game_inst->app_config.name,
        game_inst->app_config.start_pos_x,
        game_inst->app_config.start_pos_y,
        game_inst->app_config.start_width,
        game_inst->app_config.start_height)) {
        return false;
    }
    // Renderer system
    renderer_system_initialize(&app_state->renderer_system_memory_requirement, 0, 0);
    app_state->renderer_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_system_memory_requirement);
    if (!renderer_system_initialize(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, game_inst->app_config.name)) {
        KFATAL("Failed to initialize renderer. Aborting application.");
        return false;
    }

     //init game
     if(!app_state->game_inst->initialize(app_state->game_inst)){
        KFATAL("Game failed to initialize.");
        return false;
     }
     // Call resize once to ensure the proper size has been set.
     app_state->game_inst->on_resize(app_state->game_inst,app_state->width,app_state->height);

     return true;
}

b8 application_run(){
    app_state->is_running=true;
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time=app_state->clock.elapsed;

    f64 running_time=0;
    u8 frame_count=0;
    f64 target_frame_seconds=1.0f/60;

    KINFO(get_memory_usage_str());
    
    while(app_state->is_running){
        if(!platform_pump_messages()){
            app_state->is_running=false;
        }

        if(!app_state->is_suspended){
            // Update clock and get delta time.
            clock_update(&app_state->clock);
            f64 current_time = app_state->clock.elapsed;
            f64 delta = (current_time - app_state->last_time);
            f64 frame_start_time = platform_get_absolute_time();

            if(!app_state->game_inst->update(app_state->game_inst,(f32)delta)){
                KFATAL("game update failed");
                app_state->is_running=false;
                break;
            }
            //call the render routine
            if(!app_state->game_inst->render(app_state->game_inst,(f32)delta)){
                KFATAL("Game render failed");
                app_state->is_running=false;
                break;
            }
             // TODO: refactor packet creation
             render_packet packet;
             packet.delta_time = delta;
             renderer_draw_frame(&packet);

            // Figure out how long the frame took and, if below
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;
            if (remaining_seconds > 0) {
                u64 remaining_ms = (remaining_seconds * 1000);  
                // If there is time left, give it back to the OS.
                b8 limit_frames = false;
                if (remaining_ms > 0 && limit_frames) {
                    platform_sleep(remaining_ms - 1);
                }   
                frame_count++;
            }
            
            // NOTE: Input update/state copying should always be handled
            // after any input should be recorded; I.E. before this line.
            // As a safety, input is the last thing to be updated before
            // this frame ends.
            input_update(delta);

            //update last time
            app_state->last_time=current_time;
        }
    }
    app_state->is_running=false;

    // Shutdown event system.
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    
    input_system_shutdown(app_state->input_system_state);

    renderer_system_shutdown(app_state->renderer_system_state);

    platform_system_shutdown(app_state->platform_system_state);
    
    memory_system_shutdown(app_state->memory_system_state);

    event_system_shutdown(app_state->event_system_state);

    return true;
}
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            app_state->is_running = false;
            return true;
        }
    }

    return false;
}
void application_get_framebuffer_size(u32* width, u32* height) {
    *width = app_state->width;
    *height = app_state->height;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            KDEBUG("Explicit - A key pressed!");
        } else {
            KDEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            KDEBUG("Explicit - B key released!");
        } else {
            KDEBUG("'%c' key released in window.", key_code);
        }
    }
    return false;
}

b8 application_on_resize(u16 code,void* sender,void* listener_inst, event_context context){
    if(code==EVENT_CODE_RESIZED){
        u16 width=context.data.u16[0];
        u16 height=context.data.u16[1];
        // check if different
        if(width!=app_state->width || height!=app_state->height){
            app_state->width=width;
            app_state->height=height;
            KDEBUG("Window resize: %i,%i", width, height);
            
            //handle minimization
            if(width==0 || height==0){
                KINFO("window minimized, suspending application");
                app_state->is_suspended=true;
                return true;
            }else{
                if(app_state->is_suspended){
                    KINFO("window restored");
                    app_state->is_suspended=false;
                }
                app_state->game_inst->on_resize(app_state->game_inst,width,height);
                renderer_on_resized(width,height);
            }
        }
    }
    return false;//not allow other listeners
}