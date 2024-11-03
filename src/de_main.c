//
// @info(mg) The idea is that this file should contain game logic
//           and it should be isolated from platform specific
//           stuff when it's reasonable.
//
static void Game_AdvanceSimulation(AppState *app)
{
    // update prev_p
    ForU32(obj_id, app->object_count)
    {
        Object *obj = app->object_pool + obj_id;
        obj->prev_p = obj->p;
    }

    // animate special wall
    {
        Object *obj = Object_Get(app, app->special_wall);
        obj->collision_rotation = WrapF(0.f, 1.f, obj->collision_rotation + app->dt);
        obj->sprite_rotation = obj->collision_rotation;
        Object_UpdateCollisionVerticesAndNormals(app, obj);
    }

    // player input
    ForArray(player_id_index, app->player_ids)
    {
        Object *player = Object_PlayerFromIndex(app, player_id_index);

        V2 dir = {0};
        if (player_id_index == 0)
        {
            if (app->keyboard[SDL_SCANCODE_W] || app->keyboard[SDL_SCANCODE_UP])    dir.y += 1;
            if (app->keyboard[SDL_SCANCODE_S] || app->keyboard[SDL_SCANCODE_DOWN])  dir.y -= 1;
            if (app->keyboard[SDL_SCANCODE_A] || app->keyboard[SDL_SCANCODE_LEFT])  dir.x -= 1;
            if (app->keyboard[SDL_SCANCODE_D] || app->keyboard[SDL_SCANCODE_RIGHT]) dir.x += 1;
        }
        else if (player_id_index == 1)
        {
            // for vim fans
            if (app->keyboard[SDL_SCANCODE_K]) dir.y += 1;
            if (app->keyboard[SDL_SCANCODE_J]) dir.y -= 1;
            if (app->keyboard[SDL_SCANCODE_H]) dir.x -= 1;
            if (app->keyboard[SDL_SCANCODE_L]) dir.x += 1;
        }
        dir = V2_Normalize(dir);

        bool ice_skating_dlc = (player_id_index == 1);
        if (ice_skating_dlc)
        {
            // @todo(mg): this is bad, we need a fixed timestep
            float player_speed = 0.005f * app->dt;
            V2 player_ddp = V2_Scale(dir, player_speed);
            player->dp = V2_Add(player->dp, player_ddp);

            float drag = -0.999f * app->dt;
            V2 player_drag = V2_Scale(player->dp, drag);
            player->dp = V2_Add(player->dp, player_drag);
        }
        else
        {
            float player_speed = 5.f * app->dt;
            player->dp = V2_Scale(dir, player_speed);
        }
    }

    // movement & collision
    ForU32(obj_id, app->object_count)
    {
        Object *obj = app->object_pool + obj_id;
        obj->has_collision = false;
    }
    ForU32(obj_id, app->object_count)
    {
        Object *obj = app->object_pool + obj_id;
        if (!(obj->flags & ObjectFlag_Move)) continue;

        obj->p = V2_Add(obj->p, obj->dp);
        Object_UpdateCollisionVerticesAndNormals(app, obj);

        ForU32(collision_iteration, 8) // support up to 8 overlapping wall collisions
        {
            CollisionProjectionResult minmax_obj_obj = Object_CollisionProjection(obj, obj);

            Uint32 closest_obstacle_id = 0;
            float closest_obstacle_separation_dist = FLT_MAX;
            V2 closest_obstacle_wall_normal = {0};

            ForU32(obstacle_id, app->object_count)
            {
                Object *obstacle = app->object_pool + obstacle_id;
                if (!(obstacle->flags & ObjectFlag_Collide)) continue;
                if (obj == obstacle) continue;

                float biggest_dist = -FLT_MAX;
                V2 wall_normal = {0};

                // @info(mg) SAT algorithm needs 2 iterations
                // from the perspective of the obj
                // and from the perspective of the obstacle.
                ForU32(sat_iteration, 2)
                {
                    Object *normal_obj;
                    CollisionProjectionResult a;
                    CollisionProjectionResult b;
                    if (sat_iteration == 0)
                    {
                        normal_obj = obj;
                        a = minmax_obj_obj;
                        b = Object_CollisionProjection(normal_obj, obstacle);
                    }
                    else
                    {
                        normal_obj = obstacle;
                        a = Object_CollisionProjection(normal_obj, obstacle); // @speed(mg) we could cache these per object
                        b = Object_CollisionProjection(normal_obj, obj);
                    }

                    ForArray(i, a.arr)
                    {
                        static_assert(ArrayCount(a.arr) == ArrayCount(obj->collision_normals));
                        V2 normal = normal_obj->collision_normals[i];

                        float d = RngF_MaxDistance(a.arr[i], b.arr[i]);
                        if (d > 0.f)
                        {
                            // @info(mg) We can exit early from checking this
                            //     obstacle since we found an axis that has
                            //     a separation between obj and obstacle.
                            goto skip_this_obstacle;
                        }

                        if (d > biggest_dist)
                        {
                            biggest_dist = d;
                            wall_normal = normal;
                        }
                        else if (d == biggest_dist)
                        {
                            // @info(mg) Tie break for walls that are parallel (like in rectangles).
                            //           We pick a normal from a wall that is facing the obj more.
                            //           I have a suspicion that this branch could be avoided.
                            V2 obj_dir = V2_Sub(obj->p, obstacle->p);
                            float current_inner = V2_Inner(obj_dir, wall_normal);
                            float new_inner = V2_Inner(obj_dir, normal);
                            if (new_inner > current_inner)
                            {
                                wall_normal = normal;
                            }
                        }
                    }
                }

                if (closest_obstacle_separation_dist > biggest_dist)
                {
                    closest_obstacle_separation_dist = biggest_dist;
                    closest_obstacle_wall_normal = wall_normal;
                    closest_obstacle_id = obstacle_id;
                }

                obj->has_collision |= (biggest_dist < 0.f);
                obstacle->has_collision |= (biggest_dist < 0.f);

                skip_this_obstacle:;
            }

            if (closest_obstacle_separation_dist < 0.f)
            {
                V2 move_out_dir = closest_obstacle_wall_normal;
                float move_out_magnitude = -closest_obstacle_separation_dist;

                V2 move_out = V2_Scale(move_out_dir, move_out_magnitude);
                obj->p = V2_Add(obj->p, move_out);

                // remove all velocity on collision axis
                // we might want to do something different here!
                if (move_out.x) obj->dp.x = 0;
                if (move_out.y) obj->dp.y = 0;

                Object_UpdateCollisionVerticesAndNormals(app, obj);
            }
            else
            {
                // Collision not found, stop iterating
                break;
            }
        } // collision_iteration
    } // obj_id


    // animate textures
    ForU32(obj_id, app->object_count)
    {
        Object *obj = app->object_pool + obj_id;
        if (Sprite_Get(app, obj->sprite_id)->frame_count <= 1) continue;

        Uint32 frame_index_map[8] =
        {
            0, 1, 2, 1,
            0, 3, 4, 3
        };

        bool in_idle_frame = (0 == frame_index_map[obj->sprite_animation_index]);

        float distance = V2_Length(V2_Sub(obj->p, obj->prev_p));
        float anim_speed = (16.f * app->dt);
        anim_speed += (3200.f * distance * app->dt);

        if (!distance && in_idle_frame)
        {
            anim_speed = 0.f;
        }
        obj->sprite_animation_t += anim_speed;

        float period = 1.f;
        while (obj->sprite_animation_t > period)
        {
            obj->sprite_animation_t -= period;
            obj->sprite_animation_index += 1;
        }

        obj->sprite_animation_index %= ArrayCount(frame_index_map);
        obj->sprite_frame_index = frame_index_map[obj->sprite_animation_index];
    }

    // move camera
    {
        Object *player = Object_Get(app, app->player_ids[0]);
        app->camera_p = player->p;
    }
}

static void Game_VerticesCameraTransform(AppState *app, V2 verts[4], float camera_scale, V2 window_transform)
{
    ForU32(i, 4)
    {
        // apply camera transform
        verts[i].x -= app->camera_p.x;
        verts[i].y -= app->camera_p.y;

        verts[i].x *= camera_scale;
        verts[i].y *= camera_scale;

        verts[i].x += window_transform.x;
        verts[i].y += window_transform.y;

        // fix y axis direction to +Y up (SDL uses +Y down, -Y up)
        verts[i].y = app->height - verts[i].y;
    }
}

static void Game_IssueDrawCommands(AppState *app)
{
    // animate collision overlay texture
    if (app->debug.draw_collision_box)
    {
        Sprite *sprite_overlay = Sprite_Get(app, app->sprite_overlay_id);

        float overlay_speed = 12.f;
        app->debug.collision_sprite_animation_t += overlay_speed * app->dt;

        float period = 1.f;
        while (app->debug.collision_sprite_animation_t > period)
        {
            app->debug.collision_sprite_animation_t -= period;
            app->debug.collision_sprite_frame_index += 1;
        }

        app->debug.collision_sprite_frame_index %= sprite_overlay->frame_count;
    }

    // draw objects
    {
        float camera_scale = 1.f;
        {
            float wh = Max(app->width, app->height); // pick bigger window dimension
            camera_scale = wh / app->camera_range;
        }
        V2 window_transform = (V2){app->width*0.5f, app->height*0.5f};

        ForU32(object_index, app->object_count)
        {
            Object *obj = app->object_pool + object_index;

            if (obj->dirty_sprite_vertices)
            {
                Object_CalculateVerticesAndNormals(app, obj, true);
            }

            V2 verts[4];
            static_assert(sizeof(verts) == sizeof(obj->sprite_vertices));
            memcpy(verts, obj->sprite_vertices, sizeof(obj->sprite_vertices));
            Game_VerticesCameraTransform(app, verts, camera_scale, window_transform);

            SDL_FColor fcolor = ColorF_To_SDL_FColor(obj->color);
            SDL_Vertex sdl_verts[4];
            SDL_zerop(sdl_verts);

            static_assert(ArrayCount(verts) == ArrayCount(sdl_verts));
            ForArray(i, sdl_verts)
            {
                sdl_verts[i].position = V2_To_SDL_FPoint(verts[i]);
                sdl_verts[i].color = fcolor;
            }

            Sprite *sprite = Sprite_Get(app, obj->sprite_id);
            {
                float tex_y0 = 0.f;
                float tex_y1 = 1.f;
                if (sprite->frame_count > 1)
                {
                    Uint32 frame_index = obj->sprite_frame_index;
                    float tex_height = 1.f / sprite->frame_count;
                    tex_y0 = frame_index * tex_height;
                    tex_y1 = tex_y0 + tex_height;
                }
                sdl_verts[0].tex_coord = (SDL_FPoint){0, tex_y1};
                sdl_verts[1].tex_coord = (SDL_FPoint){1, tex_y1};
                sdl_verts[2].tex_coord = (SDL_FPoint){0, tex_y0};
                sdl_verts[3].tex_coord = (SDL_FPoint){1, tex_y0};
            }

            int indices[] = { 0, 1, 2, 2, 3, 1 };
            SDL_RenderGeometry(app->renderer, sprite->tex,
                               sdl_verts, ArrayCount(sdl_verts),
                               indices, ArrayCount(indices));
        }

        if (app->debug.draw_collision_box)
        {
            ForU32(object_index, app->object_count)
            {
                Object *obj = app->object_pool + object_index;
                if (!(obj->flags & ObjectFlag_Collide)) continue;

                V2 verts[4];
                static_assert(sizeof(verts) == sizeof(obj->collision_vertices));
                memcpy(verts, obj->collision_vertices, sizeof(obj->collision_vertices));
                Game_VerticesCameraTransform(app, verts, camera_scale, window_transform);

                ColorF color = ColorF_RGBA(1, 0, 0.8f, 0.8f);
                if (obj->has_collision)
                {
                    color.g = 1;
                    color.a = 1;
                }

                SDL_FColor fcolor = ColorF_To_SDL_FColor(color);
                SDL_Vertex sdl_verts[4];
                SDL_zerop(sdl_verts);

                static_assert(ArrayCount(verts) == ArrayCount(sdl_verts));
                ForArray(i, sdl_verts)
                {
                    sdl_verts[i].position = V2_To_SDL_FPoint(verts[i]);
                    sdl_verts[i].color = fcolor;
                }

                Sprite *sprite = Sprite_Get(app, app->sprite_overlay_id);
                {
                    float tex_y0 = 0.f;
                    float tex_y1 = 1.f;
                    if (sprite->frame_count > 1)
                    {
                        Uint32 frame_index = app->debug.collision_sprite_frame_index;
                        float tex_height = 1.f / sprite->frame_count;
                        tex_y0 = frame_index * tex_height;
                        tex_y1 = tex_y0 + tex_height;
                    }
                    sdl_verts[0].tex_coord = (SDL_FPoint){0, tex_y1};
                    sdl_verts[1].tex_coord = (SDL_FPoint){1, tex_y1};
                    sdl_verts[2].tex_coord = (SDL_FPoint){0, tex_y0};
                    sdl_verts[3].tex_coord = (SDL_FPoint){1, tex_y0};
                }

                int indices[] = { 0, 1, 2, 2, 3, 1 };
                SDL_RenderGeometry(app->renderer, sprite->tex,
                                   sdl_verts, ArrayCount(sdl_verts),
                                   indices, ArrayCount(indices));
            }
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

static void Game_Iterate(AppState *app)
{
    {
        Uint64 new_frame_time = SDL_GetTicks();
        Uint64 delta_time = new_frame_time - app->frame_time;
        app->frame_time = new_frame_time;
        app->dt = delta_time * (0.001f);
        app->dt = Min(app->dt, 1.f); // clamp dt to 1s

        if (app->debug.fixed_dt)
        {
            app->dt = app->debug.fixed_dt;
        }
    }

    bool run_simulation = (!app->debug.pause_on_every_frame || !app->debug.paused_frame);
    if (run_simulation)
    {
        Game_AdvanceSimulation(app);
    }
    Game_IssueDrawCommands(app);
}

static void Game_Init(AppState *app)
{
    // init debug options
    {
        //app->debug.fixed_dt = 0.1f;
        //app->debug.pause_on_every_frame = true;
        app->debug.draw_collision_box = true;
    }

    app->frame_time = SDL_GetTicks();
    app->object_count += 1; // reserve object under index 0 as special 'nil' value
    app->sprite_count += 1; // reserve sprite under index 0 as special 'nil' value
    app->camera_range = 15;

    // this assumes we run the game from build directory
    // @todo in the future we should force CWD or query demongus absolute path etc
    Sprite *sprite_overlay = Sprite_Create(app, "../res/pxart/overlay.png", 6);
    app->sprite_overlay_id = Sprite_IdFromPointer(app, sprite_overlay);

    Sprite *sprite_crate = Sprite_Create(app, "../res/pxart/crate.png", 1);
    Sprite *sprite_dude = Sprite_Create(app, "../res/pxart/dude_walk.png", 5);
    Sprite *sprite_ref = Sprite_Create(app, "../res/pxart/reference.png", 1);

    // add player
    {
        Object *player = Object_Create(app, ObjectFlag_Draw|ObjectFlag_Move|ObjectFlag_Collide);
        player->p.x = -1.f;
        float scale = 0.0175f;
        player->collision_dim.x = sprite_dude->tex->w * scale;
        player->collision_dim.y = (sprite_dude->tex->h / 4.f) * scale;
        player->color = ColorF_RGB(1,1,1);
        //player->rotation = 0.3f;
        player->sprite_id = Sprite_IdFromPointer(app, sprite_dude);
        app->player_ids[0] = Object_IdFromPointer(app, player);
    }
    // add player2
    {
        Object *player = Object_Create(app, ObjectFlag_Draw|ObjectFlag_Move|ObjectFlag_Collide);
        player->p.x = 3.f;
        player->collision_dim.x = 0.3f;
        player->collision_dim.y = 0.9f;
        player->color = ColorF_RGB(0.4f, .4f, .94f);
        app->player_ids[1] = Object_IdFromPointer(app, player);
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

        {
            Object *rot_wall = Object_Wall(app, (V2){-off,-off*2.f}, (V2){length*0.5f, thickness});
            rot_wall->collision_rotation = rot_wall->sprite_rotation = 0.125f;
        }
        {
            float px_x = sprite_ref->tex->w;
            float px_y = sprite_ref->tex->h;
            float scale = 0.035f;
            Object *ref_wall = Object_Wall(app, (V2){0, off*0.5f}, (V2){px_x*scale, px_y*scale});
            ref_wall->color = ColorF_RGBA(1,1,1,1);
            ref_wall->collision_rotation = ref_wall->sprite_rotation = -0.125f;
            ref_wall->sprite_id = Sprite_IdFromPointer(app, sprite_ref);
        }
        {
            float px_x = sprite_crate->tex->w;
            float px_y = sprite_crate->tex->h;
            float scale = 0.015f;
            Object *crate_wall = Object_Wall(app, (V2){0, -off*0.5f},
                                             (V2){px_x*scale, px_y*scale});


            crate_wall->collision_offset = (V2){0.f, -0.1f};
            crate_wall->collision_rotation = 0.125f;
            crate_wall->color = ColorF_RGBA(1,1,1,1);
            crate_wall->sprite_id = Sprite_IdFromPointer(app, sprite_crate);
        }
        {
            Object *rot_wall = Object_Wall(app, (V2){off,-off*2.f}, (V2){length*0.5f, thickness});
            rot_wall->collision_rotation = rot_wall->sprite_rotation = 0.125f;
            app->special_wall = Object_IdFromPointer(app, rot_wall);
        }
    }

    // initialize normals (for collision) and vertices (for collision & drawing)
    ForU32(obj_id, app->object_count)
    {
        Object *obj = app->object_pool + obj_id;
        Object_UpdateCollisionVerticesAndNormals(app, obj);
    }
}
