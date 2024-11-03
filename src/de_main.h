typedef enum {
    Axis2_X,
    Axis2_Y,
    Axis2_COUNT
} Axis2;

static Axis2 Axis2_Other(Axis2 axis)
{
    return (axis == Axis2_X ? Axis2_Y : Axis2_X);
}

typedef struct
{
    SDL_Texture *tex;
    Uint32 frame_count;
} Sprite;

typedef enum {
    ObjectFlag_Draw          = (1 << 0),
    ObjectFlag_Move          = (1 << 1),
    ObjectFlag_Collide       = (1 << 2),
} ObjectFlags;

typedef struct
{
    Uint32 flags;
    V2 p; // position of center
    V2 dp; // change of p
    V2 dim;
    float rotation; // in turns; 1.0 == 360 degrees
    ColorF color;
    V2 prev_p; // position from the last frame

    // calculated after applying rotation
    V2 collision_normals[4]; // right, top, left, bottom
    V2 collision_vertices[4]; // bottom-left, bottom-right, top-left, top-right

    Uint32 sprite_id;
    float sprite_rotation;
    bool dirty_sprite_vertices;
    V2 sprite_vertices[4];

    float sprite_animation_t;
    Uint32 sprite_animation_index;
    Uint32 sprite_frame_index;

    // temp
    bool has_collision;
} Object;

typedef struct
{
    // SDL, window stuff
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height;

    // user input
    V2 mouse;
    SDL_MouseButtonFlags mouse_keys;
    bool keyboard[SDL_SCANCODE_COUNT]; // true == key is down

    // time
    Uint64 frame_time;
    float dt;

    // objects
    Object object_pool[4096];
    Uint32 object_count;
    Uint32 player_ids[2];
    Uint32 special_wall;

    // sprites
    Sprite sprite_pool[32];
    Uint32 sprite_count;
    Uint32 sprite_overlay_id;

    // camera
    V2 camera_p;
    // :: camera_range ::
    // how much of the world is visible in the camera
    // float camera_scale = Max(width, height) / camera_range
    float camera_range;

    // debug
    struct {
        float fixed_dt;
        bool pause_on_every_frame;
        bool paused_frame;

        bool draw_collision_box;
        float collision_sprite_animation_t;
        Uint32 collision_sprite_frame_index;

        bool draw_texture_box;
    } debug;
} AppState;
