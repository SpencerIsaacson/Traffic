typedef unsigned int Color;

typedef struct {
    u32 width, height;
    Color *pixels;
} Texture;

typedef struct {
    int x, y, width, height;
} Rect;

Texture draw_target;


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
        draw_target.pixels[y*W+x] = texture.pixels[y*texture.width+x];
    }
}