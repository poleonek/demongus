typedef enum {
    ObjectFlag_Movement  = (1 << 0),
    ObjectFlag_Draw      = (1 << 1),
} ObjectFlags;

typedef struct
{
    Uint32 flags;
    float x, y; // position of center
    float dx, dy;
    float dim_x, dim_y;
    ColorF color;
} Object;

typedef struct
{
    // SDL, window stuff
    SDL_Window* window;
    SDL_Renderer* renderer;
    int width, height;

    // user input
    struct {
        float x, y;
    } mouse;

    // time
    Uint64 frame_time;
    float dt;

    // Entities
    Object object_pool[4096];
    Uint32 object_count;

    // Camera
    float camera_x, camera_y;
} AppState;
