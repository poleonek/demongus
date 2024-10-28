static Object *Object_Get(AppState *app, Uint32 id)
{
    Assert(app->object_count < ArrayCount(app->object_pool));
    Assert(id < app->object_count);
    return app->object_pool + id;
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

    SDL_zerop(obj);
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
    static float g = 0.5f;
    r += 0.321f;
    g += 0.111f;
    while (r > 1.f) r -= 1.f;
    while (g > 1.f) g -= 1.f;

    obj->color = ColorF_RGB(r, g, 0.5f);
    return obj;
}

static Arr4RngF Object_NormalsInnerVertices(Object *obj_normals, Object *obj_verts)
{
    Arr4RngF result;
    static_assert(ArrayCount(result.arr) == ArrayCount(obj_normals->normals));

    ForArray(normal_index, obj_normals->normals)
    {
        RngF *range = result.arr + normal_index;
        range->min = FLT_MAX;
        range->max = -FLT_MAX;

        V2 normal = obj_normals->normals[normal_index];

        ForArray(vert_index, obj_verts->vertices)
        {
            V2 vert = obj_verts->vertices[vert_index];

            float inner = V2_Inner(normal, vert);
            range->min = Min(inner, range->min);
            range->max = Max(inner, range->max);
        }
    }

    return result;
}
