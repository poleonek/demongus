// ---
// Scalar math
// ---
static float LerpF(float a, float b, float t)
{
    return a + (b - a) * t;
}

// ---
// Color
// ---
typedef union
{
    struct { float r, g, b, a; };
    float E[4];
} ColorF;
static ColorF ColorF_Normalize(ColorF f)
{
    ForArray(i, f.E)
        f.E[i] = Clamp(0.f, 1.f, f.E[i]);
    return f;
}
static ColorF ColorF_Lerp(ColorF a, ColorF b, float t)
{
    ColorF res = {};
    ForArray(i, res.E)
        res.E[i] = LerpF(a.E[i], b.E[i], t);
    return res;
}

static ColorF ColorF_RGBA(float r, float g, float b, float a)
{
    return (ColorF){r,g,b,a};
}
static ColorF ColorF_RGB(float r, float g, float b)
{
    return (ColorF){r,g,b};
}
static ColorF ColorF_ChangeA(ColorF f, float a)
{
    f.a = a;
    return f;
}

// Packed version but SDL api accepts floats too
static SDL_Color ColorF_To_SDL(ColorF f)
{
    float inv = 1.f / 255.f;
    Uint32 r = (Uint32)(f.r * inv);
    Uint32 g = (Uint32)(f.g * inv);
    Uint32 b = (Uint32)(f.b * inv);
    Uint32 a = (Uint32)(f.a * inv);

    SDL_Color res = {
        r & 0xff,
        (g & 0xff) <<  8,
        (b & 0xff) << 16,
        (a & 0xff) << 24,
    };
    return res;
}
