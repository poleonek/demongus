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
        r += app->dt * 30.f;
        if (r > 255) r = 0;
        /* set the color to white */
        SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);

        //SDL_RenderFillRect(app->renderer, &mouseposrect);

    }
}

static void Game_Init(AppState *app)
{
    app->frame_time = SDL_GetTicks();
}
