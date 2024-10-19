#include <SDL3/SDL_stdinc.h>
#include <stdint.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_HEIGHT 480
#define WINDOW_WIDTH 640

typedef struct
{
    // SDL, window stuff
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height;

    // time
    Uint64 frame_time;
    float dt;
} AppState;

SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* app = (AppState*)appstate;

    {
        Uint64 new_frame_time = SDL_GetTicks();
        Uint64 delta_time = new_frame_time - app->frame_time;
        app->frame_time = new_frame_time;
        app->dt = delta_time * (0.001f);
    }

    {
        static float r = 0;
        r += app->dt * 30.f;
        if (r > 255) r = 0;
        SDL_SetRenderDrawColor(app->renderer, (int)r, 0, 0, 255);

        /* you have to draw the whole window every frame. Clearing it makes sure the whole thing is sane. */
        SDL_RenderClear(app->renderer);  /* clear whole window to that fade color. */

        /* set the color to white */
        SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);

        /* draw a square where the mouse cursor currently is. */
        //SDL_RenderFillRect(app->renderer, &mouseposrect);

        /* put everything we drew to the screen. */
        SDL_RenderPresent(app->renderer);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    AppState* state = (AppState*)appstate;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        } break;

        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS; // useful in development
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
    state->frame_time = SDL_GetTicks();

    if (!SDL_CreateWindowAndRenderer("demongus", state->width, state->height, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create window/renderer", SDL_GetError(), NULL);
        return SDL_APP_FAILURE;
    }

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
