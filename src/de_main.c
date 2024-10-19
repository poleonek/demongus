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
        app->dt = Min(app->dt, 1.f); // clamp dt to 1s
    }

    // player input
    {
        V2 dir = {0};
        if (app->keyboard[SDL_SCANCODE_W] || app->keyboard[SDL_SCANCODE_UP])    dir.y -= 1;
        if (app->keyboard[SDL_SCANCODE_S] || app->keyboard[SDL_SCANCODE_DOWN])  dir.y += 1; // @todo(mg) Y should be up!
        if (app->keyboard[SDL_SCANCODE_A] || app->keyboard[SDL_SCANCODE_LEFT])  dir.x -= 1;
        if (app->keyboard[SDL_SCANCODE_D] || app->keyboard[SDL_SCANCODE_RIGHT]) dir.x += 1;
        dir = V2_Normalize(dir);

        float player_speed = 5.f * app->dt;

        Assert(app->player_id < ArrayCount(app->object_pool));
        Object *player = app->object_pool + app->player_id;
        player->x += dir.x * player_speed;
        player->y += dir.y * player_speed;
    }

    // display objects
    {
        float camera_scale = 1.f;
        {
            float wh = Max(app->width, app->height); // pick bigger window dimension
            camera_scale = wh / app->camera_range;
        }

        float camera_transform_x = app->width*0.5f - app->camera_x;
        float camera_transform_y = app->height*0.5f - app->camera_y;

        ForU32(i, app->object_count) {
            Object *obj = app->object_pool + i;

            float dim = 20;
            SDL_FRect rect = {
                obj->x - obj->dim_x*0.5f,
                obj->y - obj->dim_y*0.5f,
                obj->dim_x,
                obj->dim_y
            };

            // apply camera
            {
                rect.x *= camera_scale;
                rect.y *= camera_scale;
                rect.w *= camera_scale;
                rect.h *= camera_scale;

                rect.x += camera_transform_x;
                rect.y += camera_transform_y;
            }

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
            app->mouse_x - 0.5f*dim, app->mouse_y - 0.5f*dim,
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
    obj->color = ColorF_RGB(1,1,1);
    return obj;
}

// @info(mg) This function will probably be replaced in the future
//           when we track 'key/index' in the object itself.
static Uint32 Object_IdFromPointer(AppState *app, Object *obj)
{
    size_t byte_delta = (size_t)obj - (size_t)app->object_pool;
    size_t id = byte_delta / sizeof(*obj);
    return (Uint32)id;
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
    app->object_count += 1; // reserve object under index 0 as special 'nil' value
    app->camera_range = 20;

    // add player
    {
        Object *player = Object_Create(app, ObjectFlag_Draw);
        player->dim_x = player->dim_y = 0.25f;
        app->player_id = Object_IdFromPointer(app, player);
    }

    // add walls
    {
        float thickness = 0.5f;
        float length = 10.f;
        float off = length*0.5f - thickness*0.5f;
        Object_Wall(app, off, 0, thickness, length);
        Object_Wall(app, -off, 0, thickness, length);
        Object_Wall(app, 0, off, length, thickness);
        Object_Wall(app, 0, -off, length, thickness);
    }
}
