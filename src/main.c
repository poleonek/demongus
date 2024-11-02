#define SDL_ASSERT_LEVEL 2
#include <SDL3/SDL_stdinc.h>
#include <stdint.h>
#include <stdio.h>
#include <float.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#define WINDOW_HEIGHT 900
#define WINDOW_WIDTH 1600

#include "de_base.h"
#include "de_math.h"
#include "de_main.h"
#include "de_object.c"
#include "de_main.c"

SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* app = (AppState*)appstate;

    // input
    {
        app->mouse_keys = SDL_GetMouseState(&app->mouse.x, &app->mouse.y);

        // keyboard state
        {
            int numkeys = 0;
            const bool *key_state_arr = SDL_GetKeyboardState(&numkeys);
            int to_copy = Min(numkeys, ArrayCount(app->keyboard));
            memcpy(app->keyboard, key_state_arr, to_copy);
        }
    }

    SDL_SetRenderDrawColor(app->renderer, 64, 64, 64, 255);
    SDL_RenderClear(app->renderer);
    Game_Iterate(app);
    SDL_RenderPresent(app->renderer);

    app->debug.paused_frame = true;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    AppState* app = (AppState*)appstate;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        } break;

        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.key == SDLK_ESCAPE)
            {
                return SDL_APP_SUCCESS; // useful in development
            }

            if (event->type == SDL_EVENT_KEY_DOWN &&
                event->key.key == SDLK_P)
            {
                app->debug.paused_frame = false;
            }
        } break;

        default:
        {
        } break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to initialize SDL3.", SDL_GetError(), NULL);
        return SDL_APP_FAILURE;
    }

    *appstate = SDL_calloc(1, sizeof(AppState));
    AppState* state = (AppState*)*appstate;
    if (!state)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to allocate appstate", SDL_GetError(), NULL);
        return SDL_APP_FAILURE;
    }
    state->width = WINDOW_WIDTH;
    state->height = WINDOW_HEIGHT;

    if (!SDL_CreateWindowAndRenderer("demongus", state->width, state->height,
                                     SDL_WINDOW_RESIZABLE, &state->window, &state->renderer))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create window/renderer", SDL_GetError(), NULL);
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);

    Game_Init(state);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    const char* error = SDL_GetError();
    if (error[0])
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", error);
    }

    // No need to check for null
    SDL_free(appstate);
}
