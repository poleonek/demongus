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

static Sprite *Sprite_Create(AppState *app, const char *texture_path,
                             Uint32 frame_count)
{
    Assert(app->sprite_count < ArrayCount(app->sprite_pool));
    Sprite *sprite = app->sprite_pool + app->sprite_count;
    app->sprite_count += 1;

    sprite->tex = IMG_LoadTexture(app->renderer, texture_path);
    SDL_SetTextureScaleMode(sprite->tex, SDL_SCALEMODE_NEAREST);

    sprite->frame_count = frame_count;
    return sprite;
}
