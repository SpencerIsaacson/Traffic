typedef unsigned int Color;

typedef struct {
    unsigned char b, g, r, a;
} Color2;

typedef struct {
    u32 width, height;
    Color *pixels;
} Texture;

typedef struct {
    int x, y, width, height;
} Rect;

Texture draw_target;

float lerp_float(float a, float b, float t) {
    return a + t * (b - a);
}

//TODO you will need a separate implementation on the pi with a different channel order
Color2 color2_from_color(Color col) {
    unsigned char *p = (unsigned char*)&col;
    return (Color2){.b = p[0], .g = p[1], .r = p[2]};
}
Color lerp_color(Color2 a, Color2 b, float t) {
    float a_r = a.r;
    float a_g = a.g;
    float a_b = a.b;
    float b_r = b.r;
    float b_g = b.g;
    float b_b = b.b;

    float r  = lerp_float(a_r, b_r, t);
    float g  = lerp_float(a_g, b_g, t);
    float _b = lerp_float(a_b, b_b, t);

    return make_color((unsigned char)r, (unsigned char)g, (unsigned char)_b, 0xFF); //ignore alpha for now
}

void fill(Color col) {
    assert(draw_target.pixels);
    for(int _y = 0; _y < draw_target.height; _y++)
    for(int _x = 0; _x < draw_target.width; _x++) {
        draw_target.pixels[_y*W+_x] = col;
    }
}

void draw_horizontal(Color col, u32 y) {
    assert(draw_target.pixels);
    for (int x = 0; x < draw_target.width; ++x)
    {
        draw_target.pixels[y*draw_target.width+x] = col;
    }
}

void draw_vertical(Color col, u32 x) {
    assert(draw_target.pixels);
    for (int y = 0; y < draw_target.height; ++y)
    {
        draw_target.pixels[y*draw_target.width+x] = col;
    }
}

void fill_rect(Color col, Rect r) {
    assert(draw_target.pixels);
    int min_x = r.x;
    int max_x = r.x + r.width;
    int min_y = r.y;
    int max_y = r.y + r.height;

    if(min_x < 0)
        min_x = 0;
    if(max_x > W)
        max_x = W;
    if(min_y < 0)
        min_y = 0;
    if(max_y > H)
        max_y = H;

    for(int _y = min_y; _y < max_y; _y++)
    for(int _x = min_x; _x < max_x; _x++) {
        draw_target.pixels[_y*W+_x] = col;
    }
}

// void draw_texture(Texture texture) {
//     assert(draw_target.pixels);
//     unsigned long long *p = (unsigned long long *)draw_target.pixels;
//     unsigned long long *p2 = (unsigned long long *)texture.pixels;
//     int f = texture.height/2;
//     int g = texture.width/2;
//     for (int y = 0; y < f; ++y)
//     for (int x = 0; x < g; ++x) {
//         p[y*W+x] = p2[y*texture.width+x];
//     }
// }

void draw_texture(Texture texture) {
    assert(draw_target.pixels);
    for (int y = 0; y < texture.height; ++y)
    for (int x = 0; x < texture.width; ++x) {
        draw_target.pixels[y*draw_target.width+x] = texture.pixels[y*texture.width+x];
    }
}

void draw_texture2(Texture texture, int x, int y) {
    assert(draw_target.pixels);
    for (int _y = 0; _y < texture.height; ++_y)
    for (int _x = 0; _x < texture.width; ++_x) {
        draw_target.pixels[(y+_y)*draw_target.width+(x+_x)] = texture.pixels[_y*texture.width+_x];
    }
}

void sample_texture_region(Texture texture, Rect r) {
    assert(draw_target.pixels);
    for (int y = 0; y < r.height; ++y)
    for (int x = 0; x < r.width; ++x) {
        draw_target.pixels[(r.y+y)*W+(r.x+x)] = texture.pixels[(r.y+y)*texture.width+(r.x+x)];
    }
}

typedef struct {
    float x, y;
} v2;

typedef struct {
    float x, y, z;
} v3f;

void clamp_int(int *val, int min, int max) {
    if(*val < min)
        *val = min;
    if(*val > max)
        *val = max;
}
float get_min3(float a, float b, float c) {
    if(a < b && a < c)
        return a;
    if(b < c && b < a)
        return b;
    if(c < b && c < a)
        return c;    
    return a;
}
float get_max3(float a, float b, float c) {
    if(a > b && a > c)
        return a;
    if(b > c && b > a)
        return b;
    if(c > b && c > a)
        return c;    
    return a;
}

v3f to_barycentric_space(float v_x, float v_y, v2 a, v2 b, v2 c)
{
    float b1, b2, b3;
    float denom = (a.y - c.y) * (b.x - c.x) + (b.y - c.y) * (c.x - a.x);

    b1 = ((v_y - c.y) * (b.x - c.x) + (b.y - c.y) * (c.x - v_x)) / denom;
    b2 = ((v_y - a.y) * (c.x - a.x) + (c.y - a.y) * (a.x - v_x)) / denom;
    b3 = ((v_y - b.y) * (a.x - b.x) + (a.y - b.y) * (b.x - v_x)) / denom;

    v3f result = { b1, b2, b3 };
    return result;
}

void fill_triangle(v2 a, v2 b, v2 c, Color color)
{
    int x_min = (int)get_min3(a.x, b.x, c.x);
    int y_min = (int)get_min3(a.y, b.y, c.y);
    int x_max = (int)get_max3(a.x, b.x, c.x);
    int y_max = (int)get_max3(a.y, b.y, c.y);

    clamp_int(&x_min, 0, W-1);
    clamp_int(&x_max, 0, W-1);
    clamp_int(&y_min, 0, H-1);
    clamp_int(&y_max, 0, H-1);

    for (int _x = x_min; _x <= x_max; _x++)
    {
        for (int _y = y_min; _y <= y_max; _y++)
        {
            v3f v = to_barycentric_space(_x, _y, a, b, c);
            bool in_triangle = !(v.x < 0 || v.y < 0 || v.z < 0);
            if(in_triangle)
            {
                int i = _y*W+_x;
                draw_target.pixels[i] = color;
            }
        }
    }   
}

v2 v2_add(v2 a, v2 b)
{
    return (v2){ a.x + b.x, a.y + b.y };
}

v2 v2_subtract(v2 a, v2 b)
{
    return (v2){ a.x - b.x, a.y - b.y };
}

float v2_magnitude(v2 v)
{
    return (float)sqrt(v.x * v.x + v.y * v.y);
}

float v2_distance(v2 a, v2 b)
{
    return v2_magnitude(v2_subtract(a,b));
}


float signed_distance_to_circle(v2 sample, v2 position, float radius)
{
  return v2_magnitude(v2_subtract(sample, position)) - radius; 
}

void fill_circle(Color color, v2 position, float radius)
{
    int x_min = (int)round(position.x - radius)-5;
    int x_max = (int)round(position.x + radius)+5;
    int y_min = (int)round(position.y - radius)-5;
    int y_max = (int)round(position.y + radius)+5;

    clamp_int(&x_min, 0, W-1);
    clamp_int(&x_max, 0, W-1);
    clamp_int(&y_min, 0, H-1);
    clamp_int(&y_max, 0, H-1);

    for (int _y = y_min; _y <= y_max; _y++)
    for (int _x = x_min; _x <= x_max; _x++)
    {
        float d = signed_distance_to_circle((v2){ _x, _y }, position, radius);

        if(d < 0)
          draw_target.pixels[_y*W+_x] = color;
#define FADE_DISTANCE 5.5f
        else if(d < FADE_DISTANCE)
        {
          draw_target.pixels[_y*W+_x] = lerp_color(color2_from_color(color), color2_from_color(draw_target.pixels[_y*W+_x]), d/FADE_DISTANCE);
        }
    }
}

v2 rotate_point(v2 p, float theta) {
    float cos_theta = cos(theta);
    float sin_theta = sin(theta);
    return (v2)
    {
        .x = p.x * cos_theta - p.y * sin_theta,
        .y = p.x * sin_theta + p.y * cos_theta,
    };
}