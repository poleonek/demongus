typedef struct
{
    float x,y; // @todo(mg) add vectors?
} Entity;

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
} AppState;
