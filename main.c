#include <SDL3/SDL_stdinc.h>
#include <stdint.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#define WINDOW_HEIGHT 480
#define WINDOW_WIDTH 640

#define BYTES_PER_PIXEL 4;
typedef struct
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height;
} AppState;

SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* state = (AppState*)appstate;

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

    if (!SDL_CreateWindowAndRenderer("demongus", state->width, state->height, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create window/renderer", SDL_GetError(), NULL);
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate)
{
    const char* error = SDL_GetError();
    if (error[0])
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", error);
    }

    // No need to check for null
    SDL_free(appstate);
}
