static Object *Object_Get(AppState *app, Uint32 id)
{
    Assert(app->object_count < ArrayCount(app->object_pool));
    Assert(id < app->object_count);
    return app->object_pool + id;
}

static bool Object_IsZero(AppState *app, Object *obj)
{
    return obj == app->object_pool + 0;
}

static Object *Object_Network(AppState *app, Uint32 network_slot)
{
    Assert(network_slot < ArrayCount(app->network_ids));
    Uint32 id = app->network_ids[network_slot];
    return Object_Get(app, id);
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

static V2 Object_GetAvgCollisionPos(Object* obj)
{
    float x_sum = 0.0f;
    float y_sum = 0.0f;
    ForArray(i, obj->collision_vertices.arr)
    {
        x_sum += obj->collision_vertices.arr[i].x;
        y_sum += obj->collision_vertices.arr[i].y;
    }
    V2 collision_avg = {0};
    collision_avg.x = x_sum / ArrayCount(obj->collision_vertices.arr);
    collision_avg.y = y_sum / ArrayCount(obj->collision_vertices.arr);

    return collision_avg;
}

static Object *Object_Wall(AppState *app, V2 p, V2 dim)
{
    Object *obj = Object_Create(app, ObjectFlag_Draw|ObjectFlag_Collide);
    obj->p = p;

    obj->vertices_relative_to_p.arr[0] = (V2){-dim.x / 2, -dim.y / 2};
    obj->vertices_relative_to_p.arr[1] = (V2){ dim.x / 2, -dim.y / 2};
    obj->vertices_relative_to_p.arr[2] = (V2){ dim.x / 2,  dim.y / 2};
    obj->vertices_relative_to_p.arr[3] = (V2){-dim.x / 2,  dim.y / 2};

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

static CollisionProjectionResult Object_CollisionProjection(Normals obj_normals, Vertices obj_verts)
{
    CollisionProjectionResult result = {0};
    static_assert(ArrayCount(result.arr) == ArrayCount(obj_normals.arr));

    ForArray(normal_index, obj_normals.arr)
    {
        RngF *sat = result.arr + normal_index;
        sat->min = FLT_MAX;
        sat->max = -FLT_MAX;
        V2 normal = obj_normals.arr[normal_index];

        ForArray(vert_index, obj_verts.arr)
        {
            V2 vert = obj_verts.arr[vert_index];

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
        return (V2){0};
    }
}

static void Object_CalculateVerticesAndNormals(Object *obj, bool update_sprite)
{
    float rotation = (update_sprite ? obj->sprite_rotation : obj->collision_rotation);

    {
        static_assert(sizeof(obj->sprite_vertices.arr) == sizeof(obj->collision_vertices.arr));
        V2 *verts = (update_sprite ? obj->sprite_vertices.arr : obj->collision_vertices.arr);
        size_t vert_count = ArrayCount(obj->collision_vertices.arr);

        ForU32(i, vert_count)
        {
            V2 vertRel = obj->vertices_relative_to_p.arr[i];
            V2 rotated = V2_Rotate(vertRel, rotation);
            V2 moved = V2_Add(rotated, obj->p);
            verts[i].x = moved.x;
            verts[i].y = moved.y;
        }
    }

    if (!update_sprite)
    {
        Uint32 vert_count = ArrayCount(obj->collision_vertices.arr);
        V2* verts = obj->collision_vertices.arr;

        // calculate last normal
        obj->collision_normals.arr[vert_count - 1] = V2_CalculateNormal(verts[vert_count - 1], verts[0]);
        ForU32(vert_id, vert_count - 1) // calculate the rest
        {
            obj->collision_normals.arr[vert_id] = V2_CalculateNormal(verts[vert_id], verts[vert_id + 1]);
        }
    }
}

static void Object_UpdateCollisionVerticesAndNormals(Object *obj)
{
    // @todo This function should be called automatically?
    //       We should add some asserts and checks to make sure
    //       that we aren't using stale vertices & normals
    obj->dirty_sprite_vertices = true;
    Object_CalculateVerticesAndNormals(obj, false);
}
