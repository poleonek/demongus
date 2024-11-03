static Object *Object_Get(AppState *app, Uint32 id)
{
    Assert(app->object_count < ArrayCount(app->object_pool));
    Assert(id < app->object_count);
    return app->object_pool + id;
}
static Object *Object_PlayerFromIndex(AppState *app, Uint32 player_index)
{
    Assert(player_index < ArrayCount(app->player_ids));
    Uint32 player_id = app->player_ids[player_index];
    return Object_Get(app, player_id);
}

// @info(mg) This function will probably be replaced in the future
//           when we track 'key/index' in the object itself.
static Uint32 Object_IdFromPointer(AppState *app, Object *obj)
{
    size_t byte_delta = (size_t)obj - (size_t)app->object_pool;
    size_t id = byte_delta / sizeof(*obj);
    Assert(id < ArrayCount(app->object_pool));
    return (Uint32)id;
}

static Object *Object_Create(AppState *app, Uint32 flags)
{
    Assert(app->object_count < ArrayCount(app->object_pool));
    Object *obj = app->object_pool + app->object_count;
    app->object_count += 1;

    SDL_zerop(obj);
    obj->flags = flags;
    obj->color = ColorF_RGB(1,1,1);
    obj->sprite_scale = 1.f;
    return obj;
}

static Object *Object_Wall(AppState *app, V2 p, V2 dim)
{
    Object *obj = Object_Create(app, ObjectFlag_Draw|ObjectFlag_Collide);
    obj->p = p;
    obj->collision_dim = dim;

    static float r = 0.f;
    static float g = 0.5f;
    r += 0.321f;
    g += 0.111f;
    while (r > 1.f) r -= 1.f;
    while (g > 1.f) g -= 1.f;

    obj->color = ColorF_RGB(r, g, 0.5f);
    return obj;
}

typedef struct
{
    RngF arr[4];
} CollisionProjectionResult;

static CollisionProjectionResult Object_CollisionProjection(Object *obj_normals, Object *obj_verts)
{
    CollisionProjectionResult result = {0};
    static_assert(ArrayCount(result.arr) == ArrayCount(obj_normals->collision_normals));

    ForArray(normal_index, obj_normals->collision_normals)
    {
        RngF *sat = result.arr + normal_index;
        sat->min = FLT_MAX;
        sat->max = -FLT_MAX;
        V2 normal = obj_normals->collision_normals[normal_index];

        ForArray(vert_index, obj_verts->collision_vertices)
        {
            V2 vert = obj_verts->collision_vertices[vert_index];

            float inner = V2_Inner(normal, vert);
            sat->min = Min(inner, sat->min);
            sat->max = Max(inner, sat->max);
        }
    }

    return result;
}

static V2 Object_GetDrawDim(AppState *app, Object *obj)
{
    Sprite *sprite = Sprite_Get(app, obj->sprite_id);
    if (sprite->tex)
    {
        V2 sprite_dim =
        {
            (float)sprite->tex->w,
            (float)sprite->tex->h,
        };
        if (sprite->frame_count)
        {
            sprite_dim.y /= (float)sprite->frame_count;
        }

        float scale = obj->sprite_scale * ScaleMetersPerPixel;
        return V2_Scale(sprite_dim, scale);
    }
    else
    {
        return obj->collision_dim;
    }
}

static void Object_CalculateVerticesAndNormals(AppState *app, Object *obj, bool update_sprite)
{
    float rotation = (update_sprite ? obj->sprite_rotation : obj->collision_rotation);
    // Ideally we would have custom made sin/cos that work with turns
    // turns -> * pi -> radians
    float s = SinF(rotation * 2.f*SDL_PI_F);
    float c = CosF(rotation * 2.f*SDL_PI_F);

    if (!update_sprite)
    {
        obj->collision_normals[0] = (V2){ c,  s}; // RIGHT
        obj->collision_normals[1] = (V2){-s,  c}; // TOP
        obj->collision_normals[2] = (V2){-c, -s}; // LEFT
        obj->collision_normals[3] = (V2){ s, -c}; // BOTTOM
    }

    static_assert(sizeof(obj->sprite_vertices) == sizeof(obj->collision_vertices));
    V2 *verts = (update_sprite ? obj->sprite_vertices : obj->collision_vertices);
    size_t vert_count = ArrayCount(obj->collision_vertices);

    V2 dim = (update_sprite ? Object_GetDrawDim(app, obj) : obj->collision_dim);
    V2 half = V2_Scale(dim, 0.5f);
    verts[0] = (V2){-half.x, -half.y}; // BOTTOM-LEFT
    verts[1] = (V2){ half.x, -half.y}; // BOTTOM-RIGHT
    verts[2] = (V2){-half.x,  half.y}; // TOP-LEFT
    verts[3] = (V2){ half.x,  half.y}; // TOP-RIGHT

    ForU32(i, vert_count)
    {
        V2 vert = verts[i];

        verts[i].x = vert.x * c - vert.y * s;
        verts[i].y = vert.x * s + vert.y * c;

        verts[i].x += obj->p.x;
        verts[i].y += obj->p.y;

        if (!update_sprite)
        {
            verts[i].x += obj->collision_offset.x;
            verts[i].y += obj->collision_offset.y;
        }
    }
}

static void Object_UpdateCollisionVerticesAndNormals(AppState *app, Object *obj)
{
    // @todo This function should be called automatically?
    //       We should add some asserts and checks to make sure
    //       that we aren't using stale vertices & normals
    obj->dirty_sprite_vertices = true;
    Object_CalculateVerticesAndNormals(app, obj, false);
}
