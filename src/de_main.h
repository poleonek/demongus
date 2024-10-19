typedef enum {
    ObjectFlag_Movement  = (1 << 0),
    ObjectFlag_Draw      = (1 << 1),
} ObjectFlags;

typedef struct
{
    Uint32 flags;
    V2 p; // position of center
    V2 dim;
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

    // Entities
    Object object_pool[4096];
    Uint32 object_count;
    Uint32 player_id;

    // Camera
    V2 camera_p;
    // :: camera_range ::
    // how much of the world is visible in the camera
    // float camera_scale = Max(width, height) / camera_range
    float camera_range;

} AppState;
