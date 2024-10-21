typedef enum {
    Axis2_X,
    Axis2_Y,
    Axis2_COUNT
} Axis2;

static Axis2 Axis2_Other(Axis2 axis)
{
    return (axis == Axis2_X ? Axis2_Y : Axis2_X);
}

typedef enum {
    ObjectFlag_Draw    = (1 << 0),
    ObjectFlag_Move    = (1 << 1),
    ObjectFlag_Collide = (1 << 2),
} ObjectFlags;

typedef struct
{
    Uint32 flags;
    V2 p; // position of center
    V2 dp; // change of p
    V2 dim;
    float rot; // rotation in turns; 1.0 -> 360 degrees
    ColorF color;
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

    // Objects
    Object object_pool[4096];
    Uint32 object_count;
    Uint32 player_ids[2];

    // Camera
    V2 camera_p;
    // :: camera_range ::
    // how much of the world is visible in the camera
    // float camera_scale = Max(width, height) / camera_range
    float camera_range;
} AppState;
