//
// @info(mg) The idea is that this file should contain game logic
//           and it should be isolated from platform specific
//           stuff when it's reasonable.
//

static void Game_Iterate(AppState *app)
{
    {
        Uint64 new_frame_time = SDL_GetTicks();
        Uint64 delta_time = new_frame_time - app->frame_time;
        app->frame_time = new_frame_time;
        app->dt = delta_time * (0.001f);
    }

    {
        static float r = 0;
        r += app->dt * 90.f;
        if (r > 255) r = 0;
        /* set the color to white */
        SDL_SetRenderDrawColor(app->renderer, (int)r, 255 - (int)r, 255, 255);

        float dim = 20;
        SDL_FRect rect = {app->mouse.x - 0.5f*dim, app->mouse.y - 0.5f*dim, dim, dim};
        SDL_RenderFillRect(app->renderer, &rect);
    }
}

static void Game_Init(AppState *app)
{
    app->frame_time = SDL_GetTicks();
}
