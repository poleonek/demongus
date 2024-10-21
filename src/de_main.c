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
        if (app->keyboard[SDL_SCANCODE_W] || app->keyboard[SDL_SCANCODE_UP])    dir.y += 1;
        if (app->keyboard[SDL_SCANCODE_S] || app->keyboard[SDL_SCANCODE_DOWN])  dir.y -= 1;
        if (app->keyboard[SDL_SCANCODE_A] || app->keyboard[SDL_SCANCODE_LEFT])  dir.x -= 1;
        if (app->keyboard[SDL_SCANCODE_D] || app->keyboard[SDL_SCANCODE_RIGHT]) dir.x += 1;
        dir = V2_Normalize(dir);

        Assert(app->player_id < ArrayCount(app->object_pool));
        Object *player = app->object_pool + app->player_id;

        float player_speed = 0.001f * app->dt;
        V2 player_ddp = V2_Scale(dir, player_speed);
        player->dp = V2_Add(player->dp, player_ddp);

        float drag = -0.8f * app->dt;
        V2 player_drag = V2_Scale(player->dp, drag);
        player->dp = V2_Add(player->dp, player_drag);
    }

    // movement
    {
        float wall_margin = 0.001f;
        // @speed(mg) This could be speed up in multiple ways.
        //            We could spatially partition the world into chunks
        //            to avoid n^2 scan through all possible world objects.
        //            We just don't care about this for now.
        ForU32(obj_id, app->object_count)
        {
            Object *obj = app->object_pool + obj_id;
            if (!(obj->flags & ObjectFlag_Move)) continue;
            if (!obj->dp.x && !obj->dp.y) continue;

            // @todo(mg) iterate through multiple collisions - otherwise the player will get into the walls on complex geometry
            bool moved = false;
            ForU32(obstacle_id, app->object_count)
            {
                Object *obstacle = app->object_pool + obstacle_id;
                if (!(obstacle->flags & ObjectFlag_Collide)) continue;
                if (obj == obstacle) continue;
                // @speed(mg) early exist if obstacle is not in obj movement's bounding box

                V2 minkowski_half_dim = V2_Scale(V2_Add(obj->dim, obstacle->dim), 0.5f);

                ForU32(main_axis, Axis2_COUNT)
                {
                    Axis2 other_axis = (main_axis == Axis2_X ? Axis2_Y : Axis2_X);

                    if (obj->dp.E[main_axis])
                    {
                        float near_wall_main = obstacle->p.E[main_axis] -
                            minkowski_half_dim.E[main_axis]*SignF(obj->dp.E[main_axis]);
                        float near_wall_other0 = obstacle->p.E[other_axis] - minkowski_half_dim.E[other_axis];
                        float near_wall_other1 = obstacle->p.E[other_axis] + minkowski_half_dim.E[other_axis];

                        float obj_wall_distance = near_wall_main - obj->p.E[main_axis];
                        float collision_t = obj_wall_distance / obj->dp.E[main_axis];

                        // @todo(mg) special handle case where: collision_t < (1.f - some_epsilon)
                        // @todo(mg) make sure we handle the case where player is in the wall

                        if (collision_t > 0.f && collision_t < 1.f)
                        {
                            V2 new_p = V2_Add(obj->p, obj->dp);
                            if (new_p.E[other_axis] >= near_wall_other0 &&
                                new_p.E[other_axis] <= near_wall_other1)
                            {
                                new_p.E[main_axis] = near_wall_main - wall_margin*SignF(obj->dp.E[main_axis]);

                                obj->p.E[main_axis] = new_p.E[main_axis];
                                obj->dp.E[main_axis] = 0.f; // @todo(mg) this is not ideal, angled walls will be "sticky", we should calculate new dp vector here
                                moved = true;
                                break; // @todo find the closest collision instead of finding the first
                            }
                        }
                    }
                }
            }

            obj->p = V2_Add(obj->p, obj->dp);
        }
    }

    // move camera
    {
        Object *player = app->object_pool + app->player_id;
        app->camera_p = player->p;
    }

    // display objects
    {
        float camera_scale = 1.f;
        {
            float wh = Max(app->width, app->height); // pick bigger window dimension
            camera_scale = wh / app->camera_range;
        }
        V2 window_transform = (V2){app->width*0.5f, app->height*0.5f};

        ForU32(i, app->object_count)
        {
            Object *obj = app->object_pool + i;

            float dim = 20;
            SDL_FRect rect =
            {
                obj->p.x - obj->dim.x*0.5f,
                obj->p.y - obj->dim.y*0.5f,
                obj->dim.x,
                obj->dim.y
            };

            // apply camera
            {
                rect.x -= app->camera_p.x;
                rect.y -= app->camera_p.y;

                rect.x *= camera_scale;
                rect.y *= camera_scale;
                rect.w *= camera_scale;
                rect.h *= camera_scale;

                rect.x += window_transform.x;
                rect.y += window_transform.y;
            }

            // (SDL Y is down) -> (game world Y is up) transform
            rect.y = app->height - rect.y - rect.h;

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

// @info(mg) This function will probably be replaced in the future
//           when we track 'key/index' in the object itself.
static Uint32 Object_IdFromPointer(AppState *app, Object *obj)
{
    size_t byte_delta = (size_t)obj - (size_t)app->object_pool;
    size_t id = byte_delta / sizeof(*obj);
    return (Uint32)id;
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

static Object *Object_Wall(AppState *app, V2 p, V2 dim)
{
    Object *obj = Object_Create(app, ObjectFlag_Draw|ObjectFlag_Collide);
    obj->p = p;
    obj->dim = dim;

    static float r = 0.f;
    r += 0.421f;
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
        Object *player = Object_Create(app, ObjectFlag_Draw|ObjectFlag_Move);
        player->dim.x = 0.5f;
        player->dim.y = 0.7f;
        player->color = ColorF_RGB(.97f, .09f ,0);
        app->player_id = Object_IdFromPointer(app, player);
    }

    // add walls
    {
        float thickness = 0.5f;
        float length = 7.5f;
        float off = length*0.5f - thickness*0.5f;
        Object_Wall(app, (V2){off, 0}, (V2){thickness, length});
        Object_Wall(app, (V2){-off, 0}, (V2){thickness, length});
        Object_Wall(app, (V2){0, off}, (V2){length, thickness});
        Object_Wall(app, (V2){0,-off}, (V2){length*0.5f, thickness});
    }
}
