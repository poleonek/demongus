// @info(mg) This function will probably be replaced in the future
//           when we track 'key/index' in the sprite itself
//           (would be useful for dynamic in/out streaming of sprites).
static Uint32 Sprite_IdFromPointer(AppState *app, Sprite *sprite)
{
    size_t byte_delta = (size_t)sprite - (size_t)app->sprite_pool;
    size_t id = byte_delta / sizeof(*sprite);
    Assert(id < ArrayCount(app->sprite_pool));
    return (Uint32)id;
}

static Sprite *Sprite_Get(AppState *app, Uint32 sprite_id)
{
    Assert(sprite_id < ArrayCount(app->sprite_pool));
    return app->sprite_pool + sprite_id;
}

static Sprite *Sprite_CreateNoTex(AppState *app, Col_Vertices collision_vertices)
{
    Assert(app->sprite_count < ArrayCount(app->sprite_pool));
    Sprite *sprite = app->sprite_pool + app->sprite_count;
    app->sprite_count += 1;

    sprite->collision_vertices = collision_vertices;

    Uint64 vert_count = ArrayCount(sprite->collision_vertices.arr);
    ForU64(vert_id, vert_count)
    {
        Uint64 next_vert_id = vert_id + 1;
        if (next_vert_id >= vert_count)
            next_vert_id -= vert_count;

        sprite->collision_normals.arr[vert_id]
            = V2_CalculateNormal(sprite->collision_vertices.arr[vert_id],
                                 sprite->collision_vertices.arr[next_vert_id]);
    }

    return sprite;
}

static Sprite *Sprite_Create(AppState *app, const char *texture_path, Uint32 tex_frames)
{
    SDL_Texture *tex = IMG_LoadTexture(app->renderer, texture_path);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

    V2 tex_half_dim = {(float)tex->w, (float)tex->h};
    tex_half_dim = V2_Scale(tex_half_dim, 0.5f);

    Col_Vertices default_col_verts = {0};
    default_col_verts.arr[0] = (V2){-tex_half_dim.x, -tex_half_dim.y};
    default_col_verts.arr[1] = (V2){ tex_half_dim.x, -tex_half_dim.y};
    default_col_verts.arr[2] = (V2){-tex_half_dim.x,  tex_half_dim.y};
    default_col_verts.arr[3] = (V2){ tex_half_dim.x,  tex_half_dim.y};

    Sprite *sprite = Sprite_CreateNoTex(app, default_col_verts);
    sprite->tex = tex;
    sprite->tex_frames = tex_frames;
    return sprite;
}
