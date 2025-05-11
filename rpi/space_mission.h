typedef enum {
    GameState_SPLASHSCREEN,
    GameState_TITLESCREEN,
    GameState_LEVELSELECT,
    GameState_PLAY,
    GameState_PAUSE,
    GameState_MISSIONCOMPLETE,
    GameState_count,
} GameState;

typedef struct {
    bool up, down, left, right;
    bool fire;
    bool start;
} Input;

typedef struct {
    float x, y,
    velocity_x, velocity_y,
    angle,
    size;
} SpaceThing;

typedef struct {
    SpaceThing thing; //anonymous space thing
    float thrust;
    float fuel;
} Ship;

typedef struct {
    SpaceThing thing; //anonymous space thing
    u32 health;
} Asteroid;

typedef struct {
    unsigned int asteroids;
    unsigned int enemies;
    bool stationary;
} LevelDefinition;

LevelDefinition LEVELS[] = {
    { .asteroids = 1, .enemies = 0, .stationary = true }, //stationary bool is kind of a hack, just set the velocity to zero
    { .asteroids = 3, .enemies = 0 },
    { .asteroids = 2, .enemies = 1 },
    { .asteroids = 4, .enemies = 2 },
    { .asteroids = 5, .enemies = 3 },
};


typedef struct {
    int current_level;
    u32 unlocked_levels;
    GameState current_game_state;
    Input input;
    Input prev_input;
    Ship player;
#define MAX_ASTEROIDS 5    
    u32 asteroid_count;
    Asteroid asteroids[MAX_ASTEROIDS];
#define MAX_LASERS 100
    u32 laser_count;
    SpaceThing lasers[MAX_LASERS];
    float laser_lives[MAX_LASERS]; //didn't want to make another struct
    float t;
} Globals;

Globals g;

u32 rand_seed = 1;
u32 random() {
    rand_seed = rand_seed * 1664525 + 1013904223;
    return rand_seed;
}

float random_float() {
    return random() / 4294967296.0f;
}


bool collide(SpaceThing a, SpaceThing b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy) < ((a.size + b.size) / 2);
}

void generate_asteroids() {
    for (int i = 0; i < g.asteroid_count; ++i)
    {
        g.asteroids[i] = (Asteroid) {
            .thing.x = random() * W,
            .thing.y = random() * H,
            .thing.velocity_x = (random_float() - 0.5) * 2,
            .thing.velocity_y = (random_float() - 0.5) * 2,
            .thing.angle = 0,
            .health = 5,
            .thing.size = 60 + random_float() * 30,
        };
    }    
}
void init() {
    g = (Globals){
        .current_game_state = GameState_TITLESCREEN,
        .player.fuel = 100,
        .player.thing.size = 10,
        .player.thing.angle = -M_PI / 2,
    };

    g.asteroid_count = MAX_ASTEROIDS;

    generate_asteroids();
}

#define TIME_STEP 0.0166666667

void render();

#define white make_color(0xFF, 0xFF, 0xFF, 0xFF)
#define red   make_color(0xFF, 0, 0, 0xFF)
#define green make_color(0, 0xFF, 0, 0xFF)
#define blue  make_color(0, 0, 0xFF, 0xFF)

float lerp_float(float a, float b, float t) {
    return a + t * (b - a);
}

typedef struct {
    unsigned char b, g, r, a;
} Color2;

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

void tick() {
    switch(g.current_game_state) {
        case GameState_SPLASHSCREEN: {
            g.t += TIME_STEP;
            if(g.t > 6) {
                g.current_game_state = GameState_TITLESCREEN;
                g.t = 0;
            }
            float w = 350;
            float h = 175;
            fill_rect(lerp_color((Color2){}, (Color2){ .b = 255 }, fabs(sin(g.t/2))), (Rect){(W-w)/2.0f, (H-h)/2.0f, w, h });
        } break;
        case GameState_TITLESCREEN: {
            if(g.input.start) {
                g.current_game_state = GameState_LEVELSELECT;
            }

            fill_rect(white, (Rect){0,0,W,H});
        } break;
        case GameState_LEVELSELECT: {
            if(g.input.right) {
                g.t += TIME_STEP;
                if(g.t > .125f) {
                    g.t = 0;
                    g.current_level = (g.current_level + 1) % (g.unlocked_levels+1);
                }
            }

            if(g.input.left) {
                g.t += TIME_STEP;
                if(g.t > .125f) {
                    g.t = 0;
                    g.current_level--;
                    if(g.current_level < 0)
                        g.current_level = g.unlocked_levels;
                }
            }

            if(g.input.start && !g.prev_input.start) {
                g.current_game_state = GameState_PLAY;
                g.player.fuel = 100;
                g.player.thing.x = W / 2;
                g.player.thing.y = H / 2;

                g.asteroid_count = LEVELS[g.current_level].asteroids;
                rand_seed = 12;
                g.laser_count = 0;
                generate_asteroids();
            }

            fill_rect(0,(Rect){ 0, 0, W, H });
            fill_rect(white, (Rect){ g.current_level*25-2, H/2-2, 24, 24 });
            for (int i = 0; i < 5; ++i) {
                fill_rect(g.unlocked_levels < i ? red : blue, (Rect){ i*25, H/2, 20, 20 });
            }
        } break;
        case GameState_PLAY: {

            //player controls
            if (g.input.left)  g.player.thing.angle -= TIME_STEP*6.28f;
            if (g.input.right) g.player.thing.angle += TIME_STEP*6.28f;
            if (g.input.up)    g.player.thrust = fmin(g.player.thrust + 0.1, 1);
            if (g.input.down)  g.player.thrust = fmax(g.player.thrust - 0.1, 0);

            //player physics
            float acceleration = g.player.thrust * 0.2;
            g.player.thing.velocity_x = g.player.thing.velocity_x + cos((g.player.thing.angle)) * acceleration;
            g.player.thing.velocity_y = g.player.thing.velocity_y + sin((g.player.thing.angle)) * acceleration;
            #define drag (0.98f)
            g.player.thing.velocity_x = g.player.thing.velocity_x * drag;
            g.player.thing.velocity_y = g.player.thing.velocity_y * drag;

            g.player.thing.x = g.player.thing.x + g.player.thing.velocity_x;
            g.player.thing.y = g.player.thing.y + g.player.thing.velocity_y;

            // wrap around screen
            if (g.player.thing.x < 0) g.player.thing.x = W;
            if (g.player.thing.x > W) g.player.thing.x = 0;
            if (g.player.thing.y < 0) g.player.thing.y = H;
            if (g.player.thing.y > H) g.player.thing.y = 0;

            // fuel consumption
            g.player.fuel = g.player.fuel - g.player.thrust * 0.1;
            if(g.player.fuel <= 0)
                g.current_game_state = GameState_LEVELSELECT;

            // laser lifetime
            for (int i = 0; i < g.laser_count; ++i) {
                g.laser_lives[i] -= TIME_STEP;
                if(g.laser_lives[i] <= 0){
                    g.lasers[i] = g.lasers[g.laser_count-1];
                    g.laser_lives[i] = g.laser_lives[g.laser_count-1];
                    g.laser_count--;
                }
            }

            // fire laser
            if (g.input.fire) {
                if(g.laser_count < MAX_LASERS) {
                    g.lasers[g.laser_count] = (SpaceThing) {
                      .x = g.player.thing.x+cos((g.player.thing.angle))*8,
                      .y = g.player.thing.y+sin((g.player.thing.angle))*8,
                      .angle = g.player.thing.angle,
                      .velocity_x = g.player.thing.velocity_x + cos((g.player.thing.angle)) * 4,
                      .velocity_y = g.player.thing.velocity_y + sin((g.player.thing.angle)) * 4,
                    };
                    g.laser_lives[g.laser_count] = 2;
                    g.laser_count++;
                }
            }

            //move asteroids
            for (int i = 0; i < g.asteroid_count; ++i) {
                #define ast g.asteroids[i]
                ast.thing.x = ((int)(ast.thing.x + ast.thing.velocity_x + W) % W);
                ast.thing.y = ((int)(ast.thing.y + ast.thing.velocity_y + H) % H);
                ast.thing.angle = ast.thing.angle + .02;
                #undef ast
            }

            //move lasers
            for (int i = 0; i < g.laser_count; ++i) {
                #define laser g.lasers[i]
                laser.x = laser.x + laser.velocity_x;
                laser.y = laser.y + laser.velocity_y;                
                if (laser.x < 0) laser.x = W;
                if (laser.x > W) laser.x = 0;
                if (laser.y < 0) laser.y = H;
                if (laser.y > H) laser.y = 0;
                #undef laser
            }

            //check laser:asteroid collisions
            for (int o = 0; o < g.asteroid_count; ++o)
            for (int i = 0; i < g.laser_count; ++i) {
                SpaceThing laser = g.lasers[i];
                SpaceThing ast   = g.asteroids[o].thing;

                if(collide(laser, ast)) {
                    g.lasers[i] = g.lasers[g.laser_count-1];
                    g.laser_lives[i] = g.laser_lives[g.laser_count-1];
                    g.laser_count--;
                    g.asteroids[o].health--;
                    if(g.asteroids[o].health <= 0){
                        g.asteroids[o] = g.asteroids[g.asteroid_count-1];
                        g.asteroid_count--;
                    }
                    goto break1;
                }
            }
            break1:

            //check player collisions
            {
                for (int i = 0; i < g.asteroid_count; ++i) {
                    if(collide(g.player.thing, g.asteroids[i].thing)) {
                        g.current_game_state = GameState_LEVELSELECT;
                    }
                }
            }

            //check for victory
            {
                if(g.asteroid_count <= 0) {
                    if(g.unlocked_levels < 4 && g.current_level == g.unlocked_levels) {
                        g.unlocked_levels++;
                    }
                    g.current_game_state = GameState_MISSIONCOMPLETE;
                    g.t = 0;
                }
            }

            render();
        } break;
        case GameState_MISSIONCOMPLETE: {
            g.t += TIME_STEP;
            if(g.t > 3) {
                g.current_game_state = GameState_LEVELSELECT;
            }

            float w = 350;
            float h = 175;
            fill_rect(0, (Rect){(W-w)/2.0f, (H-h)/2.0f, w, h });
        } break;
        default:
            break;
    }
}

clock_t last = 0;
double elapsed = 0;
void simulate() {
    elapsed += (clock()-last)/((double)CLOCKS_PER_SEC);
    while(elapsed > TIME_STEP) {
        poll_input();
        tick();
        g.prev_input = g.input;
        elapsed -= TIME_STEP;
    }
    last = clock();
}

void health_bar(Color2 from, Color2 to, Rect r, float max_width) {
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

    int other = r.x + max_width;

    for(int _y = min_y; _y < max_y; _y++)
    for(int _x = min_x; _x < max_x; _x++) {
        Color col = lerp_color(from, to, (_x - min_x) / (float)(other
         - min_x));
        draw_target.pixels[_y*W+_x] = col;
    }
}

double pow_simple(double x, double y) {
    if (x <= 0.0) {
        return 0.0; // or handle as special case
    }
    return exp(y * log(x));
}

float extreme_sine(float t) {
    return pow_simple(sin(t), 88);
};

void gradient() {
    for(int _y = 0; _y < H; _y++)
    for(int _x = 0; _x < W; _x++) {
        float t = fabs(sin(_y/(float)H*3.0f));
        draw_target.pixels[_y*W+_x] = lerp_color((Color2){}, (Color2){.b = 0x55 }, t);
    }
}

Texture background;
Color background_pixels[W*H];
bool background_inited;

void render() {
    if(!background_inited) {
        Color *pixels = draw_target.pixels;
        draw_target.pixels = background_pixels;
        background = (Texture){
            .width = W, 
            .height = H,
            .pixels = background_pixels
        };

        gradient();
        rand_seed = 2;
        //static stars
        for (int i = 0; i < 100; ++i)
        {
            float x = random() % W;
            float y = random() % H;

            fill_rect(white, (Rect){ x, y, 4, 4 });
        }
        draw_target.pixels = pixels;
        background_inited = true;
    }

    draw_texture(background);

    static float t = 0;
    rand_seed = 2;
    //twinkle stars
    for (int i = 0; i < 10; ++i)
    {
        float actual_t = t + (random_float()*1000);
        u32 x = round(random_float() * W);
        u32 y = round(random_float() * H);

        float size = (extreme_sine(actual_t)*6)+1;
        float half_size = size/2.0f;
        fill_rect(white, (Rect){ x - half_size, y - half_size, size, size });
    }

    t += .01f;

    fill_rect(make_color(255,0,0,255), (Rect){ g.player.thing.x - g.player.thing.size / 2, g.player.thing.y - g.player.thing.size / 2, g.player.thing.size, g.player.thing.size });

    for (int i = 0; i < g.asteroid_count; ++i)
    {
        Asteroid ast = g.asteroids[i];        
        fill_rect(make_color(95, 50, 0, 255),
            (Rect){ 
                ast.thing.x-(ast.thing.size/2),
                ast.thing.y-(ast.thing.size/2),
                ast.thing.size, 
                ast.thing.size 
            }
        );
    }

    for (int i = 0; i < g.laser_count; ++i)
    {
        fill_rect(make_color(0, 0xFF, 0xFF, 0xFF), (Rect){ g.lasers[i].x-5, g.lasers[i].y-5, 10, 10});
    }

    //HUD
    {
        float max_width = W-20;
        float width = (g.player.fuel/100.0f)*max_width;
        health_bar( (Color2){.r = 255 }, (Color2){.g = 255 }, (Rect){ 10, H - 30, width, 20 }, max_width);
    }
}