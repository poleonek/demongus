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

    // display entities
    {
        float camera_transform_x = app->width*0.5f - app->camera_x;
        float camera_transform_y = app->height*0.5f - app->camera_y;

        ForU32(i, app->object_count) {
            Object *obj = app->object_pool + i;

            float dim = 20;
            SDL_FRect rect = {
                obj->x - obj->dim_x*0.5f + camera_transform_x,
                obj->y - obj->dim_y*0.5f + camera_transform_y,
                obj->dim_x,
                obj->dim_y
            };

            SDL_SetRenderDrawColorFloat(app->renderer, obj->color.r, obj->color.g, obj->color.b, obj->color.a);
            SDL_RenderFillRect(app->renderer, &rect);
        }
    }

    // draw mouse
    {
        static float r = 0;
        r += app->dt * 90.f;
        if (r > 255) r = 0;
        /* set the color to white */
        SDL_SetRenderDrawColor(app->renderer, (int)r, 255 - (int)r, 255, 255);

        float dim = 20;
        SDL_FRect rect = {
            app->mouse.x - 0.5f*dim, app->mouse.y - 0.5f*dim,
            dim, dim
        };
        SDL_RenderFillRect(app->renderer, &rect);
    }
}

static Object *Object_Create(AppState *app, Uint32 flags)
{
    Assert(app->object_count < ArrayCount(app->object_pool));
    Object *obj = app->object_pool + app->object_count;
    app->object_count += 1;

    MemsetZeroStructPtr(obj);
    obj->flags = flags;
    return obj;
}

static Object *Object_Wall(AppState *app, float x, float y,
                           float dim_x, float dim_y)
{
    Object *obj = Object_Create(app, ObjectFlag_Draw);
    obj->x = x;
    obj->y = y;
    obj->dim_x = dim_x;
    obj->dim_y = dim_y;

    static float r = 0.f;
    r += 0.321f;
    while (r > 1.f) r -= 1;

    obj->color = ColorF_RGB(r, .5f, .9f);
    return obj;
}

static void Game_Init(AppState *app)
{
    app->frame_time = SDL_GetTicks();

    // add walls
    {
        float thickness = 10.f;
        float length = 60.f;
        float off = length*0.5f - thickness*0.5f;
        Object_Wall(app, off, 0, thickness, length);
        Object_Wall(app, -off, 0, thickness, length);
        Object_Wall(app, 0, off, length, thickness);
        Object_Wall(app, 0, -off, length, thickness);
    }
}
