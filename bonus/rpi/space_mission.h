//#define PROFILING_ENABLED
#ifdef PROFILING_ENABLED
#define TIME_BLOCK(name, ...) do { \
    clock_t start_macro_time = clock(); \
    __VA_ARGS__ \
    clock_t end_macro_time = clock(); \
    clock_t macro_elapsed = end_macro_time-start_macro_time; \
    printf("%s time: %d uS\n", name, macro_elapsed); \
} while(0);
#else
#define TIME_BLOCK(name, ...) \
__VA_ARGS__
#endif

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
    SpaceThing thing;
    float thrust;
    float fuel;
} Ship;

typedef struct {
    SpaceThing thing;
    int health;
} Asteroid;

typedef struct {
    int current_level;
    u32 unlocked_levels;
    GameState current_game_state;
    Input input;
    Input prev_input;
    Ship player;
#define MAX_ASTEROIDS 5
    u32 asteroids_count;
    Asteroid asteroids[MAX_ASTEROIDS];
#define MAX_ENEMIES 5
    u32 enemies_count;
    Asteroid enemies[MAX_ENEMIES];
#define MAX_LASERS 100
    u32 lasers_count;
    SpaceThing lasers[MAX_LASERS];
    float laser_lives[MAX_LASERS]; //didn't want to make another struct
#define MAX_ENEMY_LASERS 20
    u32 enemy_lasers_count;
    SpaceThing enemy_lasers[MAX_LASERS];
    float enemy_laser_lives[MAX_LASERS]; //didn't want to make another struct    
    float t;
    int frame;
#define MAX_CLEAR_RECTS 300
    int clear_rects_count;
    Rect clear_rects[MAX_CLEAR_RECTS];
#define MAX_SPECIAL_CLEAR 100    
    int special_clear_count;
    Rect special_clear[MAX_SPECIAL_CLEAR];    
    v2 twinkle_stars[50];
} Globals;

Globals g;

typedef struct {
    u32 asteroids_count;
    Asteroid asteroids[MAX_ASTEROIDS];
    u32 enemies_count;
    Asteroid enemies[MAX_ENEMIES];
} LevelDefinition;

LevelDefinition LEVELS[] = {
    {
        .asteroids_count = 1, .enemies_count = 0,
        .asteroids = 
        { (Asteroid){ .thing.size = 60, .thing.x = W / 2, .thing.y = H / 2, .health = 20, } },
    },
    {
        .asteroids_count = 3, .enemies_count = 0,
        .asteroids =
        { 
            (Asteroid)
            { 
                .thing.size =  60, .thing.x = W / 2, .thing.y = H / 2, .thing.velocity_x = 1, .thing.velocity_y = 1, .health = 20,
            },
            (Asteroid)
            {
                .thing.size = 80, .thing.x =  W-30, .thing.y = H-320, .thing.velocity_x = -1, .thing.velocity_y = -1, .health = 40,
            },
            (Asteroid)
            {
                .thing.size = 80, .thing.x = 30, .thing.y = 30, .thing.velocity_x = 1, .thing.velocity_y = 1, .health = 40,
            },
        },
    },
    {
        .asteroids_count = 2, .enemies_count = 1,
        .asteroids =
        { 
            (Asteroid){ .thing.size =  60, .thing.x = W / 2, .thing.y = H / 2, .thing.velocity_x = 1, .thing.velocity_y = 1, .health = 20 },
            (Asteroid){ .thing.size = 120, .thing.x = 30,    .thing.y =    30, .thing.velocity_x = -1, .thing.velocity_y = -1, .health = 80 },
        },
    },
    {
        .asteroids_count = 3, .enemies_count = 2,
        .asteroids =
        {
            (Asteroid){ .thing.size =  110, .thing.x = W / 2, .thing.y = 10, .thing.velocity_x = 0, .thing.velocity_y = 2.6, .health = 70 },
            (Asteroid){ .thing.size =  70, .thing.x = W / 6, .thing.y = 40, .thing.velocity_x = 0, .thing.velocity_y = 2.4, .health = 30 },
            (Asteroid){ .thing.size =  50, .thing.x = W / 3, .thing.y = H-30, .thing.velocity_x = 0, .thing.velocity_y = 2.1, .health = 10 },
        }
    },
    {
        .asteroids_count = 5, .enemies_count = 3,
        .asteroids =
        {
            (Asteroid){ .thing.size =  40, .thing.x = W / 2, .thing.y = 10, .thing.velocity_x = 1.8, .thing.velocity_y = -1.6, .health = 10 },
            (Asteroid){ .thing.size =  70, .thing.x = W / 6, .thing.y = 40, .thing.velocity_x = 1.5, .thing.velocity_y =  -1.4, .health = 30 },
            (Asteroid){ .thing.size =  50, .thing.x = W / 3, .thing.y = H-30, .thing.velocity_x = 1.2, .thing.velocity_y = -1.1, .health = 10 },
            (Asteroid){ .thing.size =  60, .thing.x = 100, .thing.y = 130, .thing.velocity_x = 1.2, .thing.velocity_y = -2.1, .health = 20 },
            (Asteroid){ .thing.size =  60, .thing.x = 52, .thing.y = 240, .thing.velocity_x = 1.2, .thing.velocity_y = -.9, .health = 20 },
        }
    },
};

void add_clear_rect(Rect r) {
    g.clear_rects[g.clear_rects_count++] = r;
    assert(g.clear_rects_count < MAX_CLEAR_RECTS);
}

void add_special_clear(Rect r) {
    if(g.special_clear_count < MAX_SPECIAL_CLEAR-1){
        g.special_clear[g.special_clear_count++] = r;
        assert(g.special_clear_count < MAX_SPECIAL_CLEAR); // this assert came in handy
    }
}

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
    return sqrt(dx * dx + dy * dy) < ((a.size + b.size));
}

void generate_asteroids() {
    rand_seed = 700;
    for (int i = 0; i < g.asteroids_count; ++i)
    {
        g.asteroids[i] = (Asteroid) {
            .thing.x = random() * W,
            .thing.y = random() * H,
            .thing.velocity_x = (random_float() - 0.5) * 2,
            .thing.velocity_y = (random_float() - 0.5) * 2,
            .thing.angle = 0,
            .health = 50,
            .thing.size = 60 + random_float() * 30,
        };
    }    
}

void init() {
    g = (Globals){
        .current_game_state = GameState_PLAY,
        .player.fuel = 100,
        .player.thing.size = 20,
        .player.thing.angle = -M_PI / 2
    };


    g.asteroids_count = MAX_ASTEROIDS;

    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        g.enemies[i].thing.size = 20;
        g.enemies[i].thing.x = 20+(random()%200);
        g.enemies[i].thing.y = 20+(random()%200);
    }

    //g.unlocked_levels = 4;
/*
    Haven't done floating point printing support yet, 
    so this just takes a pointer to a value and reinterprets 
    the bits as a u32, so we can at least inspect the value in some form
*/
#define PRINTABLE(n) (*(unsigned int*)&n)

    // printf
    // (
    //     "state: %d, fuel: %x, size: %x, angle: %x\n", 
    //     g.current_game_state,
    //     PRINTABLE(g.player.fuel),
    //     PRINTABLE(g.player.thing.size),
    //     PRINTABLE(g.player.thing.angle)
    // );
}

#define TIME_STEP 0.0166666667

void render();

#define white make_color(0xFF, 0xFF, 0xFF, 0xFF)
#define red   make_color(0xFF, 0, 0, 0xFF)
#define green make_color(0, 0xFF, 0, 0xFF)
#define blue  make_color(0, 0, 0xFF, 0xFF)

void tick() {
    switch(g.current_game_state) {
        case GameState_SPLASHSCREEN: {
            //printf("g.t: %x\n", PRINTABLE(g.t));
            if(g.t > 6) {
                g.current_game_state = GameState_TITLESCREEN;
                g.t = 0;
            }
            float w = lerp_float(0, 150, g.t);
            float h = 175;
            fill_rect(lerp_color((Color2){}, (Color2){ .b = 255 }, fabs(sin(g.t/2))), (Rect){(W-w)/2.0f, (H-h)/2.0f, w, h });
            g.t += TIME_STEP;
            //printf("finished one splash tick\n");
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
                if(g.t > .25f || !g.prev_input.right) {
                    g.t = 0;
                    g.current_level = (g.current_level + 1) % (g.unlocked_levels+1);
                }
            }

            if(g.input.left) {
                g.t += TIME_STEP;
                if(g.t > .25f || !g.prev_input.left) {
                    g.t = 0;
                    g.current_level--;
                    if(g.current_level < 0)
                        g.current_level = g.unlocked_levels;
                }
            }

            if(g.input.start && !g.prev_input.start) {
                g.current_game_state = GameState_PLAY;
                g.frame = 0;
                g.player.fuel = 100;
                g.player.thing.x = W / 2;
                g.player.thing.y = H / 2 + 150;
                g.player.thing.angle = -M_PI_2;
                g.player.thing.velocity_x = 0;
                g.player.thing.velocity_y = 0;
                g.player.thrust = 0;
                g.asteroids_count = LEVELS[g.current_level].asteroids_count;

                for (int i = 0; i < g.asteroids_count; ++i)
                {
                    g.asteroids[i] = LEVELS[g.current_level].asteroids[i];
                }

                rand_seed = 12;
                g.lasers_count = 0;
                g.enemy_lasers_count = 0;

                g.enemies_count = LEVELS[g.current_level].enemies_count;
                for (int i = 0; i < g.enemies_count; ++i)
                {
                    g.enemies[i].health = 25;
                    g.enemies[i].thing.x = random()%W;
                    g.enemies[i].thing.y = random()%H;
                }
            }

            fill_rect(0,(Rect){ 0, 0, W, H });
            float w = 450;
            float h = 175;
            float x = (W-w)/2;
            float padding = 10;
            float w2 = w/5-padding;
            //fill_rect(make_color(255,0,255,255), (Rect){(W-w)/2, (H-h)/2, w, h });

            fill_rect(white, (Rect){ x+g.current_level*(w2+padding)-2, H/2-2, w2+4, w2+4 });
            for (int i = 0; i < 5; ++i) {
                fill_rect(g.unlocked_levels < i ? red : blue, (Rect){ x+i*(w2+padding), H/2, w2, w2 });
            }
        } break;
        case GameState_PLAY: {

            //gather clear rects
            {
                g.clear_rects_count = 0;
                add_clear_rect((Rect){ g.player.thing.x-50, g.player.thing.y-50, 100, 100 });

                for (int i = 0; i < 50; ++i) {
                    add_clear_rect((Rect){ g.twinkle_stars[i].x - 4, g.twinkle_stars[i].y - 4, 8, 8 });
                }

                for (int i = 0; i < g.asteroids_count; ++i) {
                    #define ast g.asteroids[i].thing
                    add_clear_rect((Rect){ast.x-(ast.size+6), ast.y-(ast.size+6), ast.size*2+12, ast.size*2+12 });
                    #undef ast
                }

                for (int i = 0; i < g.enemies_count; ++i) {
                    #define enemy g.enemies[i].thing
                    add_clear_rect((Rect){ enemy.x-50, enemy.y-50, 100, 100 });
                    #undef enemy
                }

                for (int i = 0; i < g.lasers_count; ++i) {
                    #define laser g.lasers[i]
                    add_clear_rect((Rect){ laser.x-10, laser.y-10, 25, 25 });
                    #undef laser
                }

                for (int i = 0; i < g.enemy_lasers_count; ++i) {
                    #define laser g.enemy_lasers[i]
                    add_clear_rect((Rect){ laser.x-10, laser.y-10, 25, 25 });
                    #undef laser
                }

                //HUD
                {
                    float padding = 30;
                    float max_width = 300;
                    float adjusted_width = (g.player.fuel/100.0f)*max_width;
                    float diff = max_width - adjusted_width;
                    add_clear_rect((Rect){ W/2-max_width/2 + adjusted_width+1, H - 30, diff, 20 });
                } 
            }

            //player controls
            if (g.input.left)  g.player.thing.angle -= TIME_STEP*6.28f;
            if (g.input.right) g.player.thing.angle += TIME_STEP*6.28f;

            if (g.input.up)    g.player.thrust = fmin(g.player.thrust + 0.1, 1);
            if (g.input.down)  g.player.thrust = fmax(g.player.thrust - 0.1, 0);

            //player physics
            Rect old_player_rect = (Rect){ g.player.thing.x-50, g.player.thing.y-50, 100, 100 };

            float acceleration = g.player.thrust * 0.2;
            g.player.thing.velocity_x = g.player.thing.velocity_x + cos((g.player.thing.angle)) * acceleration;
            g.player.thing.velocity_y = g.player.thing.velocity_y + sin((g.player.thing.angle)) * acceleration;
            #define drag (0.98f)
            g.player.thing.velocity_x = g.player.thing.velocity_x * drag;
            g.player.thing.velocity_y = g.player.thing.velocity_y * drag;

            g.player.thing.x = g.player.thing.x + g.player.thing.velocity_x;
            g.player.thing.y = g.player.thing.y + g.player.thing.velocity_y;

            // wrap around screen
            if (g.player.thing.x < 0) { add_special_clear(old_player_rect); g.player.thing.x = W; };
            if (g.player.thing.x > W) { add_special_clear(old_player_rect); g.player.thing.x = 0; };
            if (g.player.thing.y < 0) { add_special_clear(old_player_rect); g.player.thing.y = H; };
            if (g.player.thing.y > H) { add_special_clear(old_player_rect); g.player.thing.y = 0; };

            // fuel consumption
            g.player.fuel = g.player.fuel - g.player.thrust * 0.1;
            if(g.player.fuel <= 0){
                g.current_game_state = GameState_LEVELSELECT;
            }

            // laser lifetime
            for (int i = 0; i < g.lasers_count; ++i) {
                g.laser_lives[i] -= TIME_STEP;
                if(g.laser_lives[i] <= 0){
                    add_special_clear((Rect){ g.lasers[i].x-10, g.lasers[i].y-10, 25, 25 });
                    g.lasers[i] = g.lasers[g.lasers_count-1];
                    g.laser_lives[i] = g.laser_lives[g.lasers_count-1];
                    g.lasers_count--;
                }
            }

            // enemy laser lifetime
            for (int i = 0; i < g.enemy_lasers_count; ++i) {
                g.enemy_laser_lives[i] -= TIME_STEP;
                if(g.enemy_laser_lives[i] <= 0){
                    add_special_clear((Rect){ g.enemy_lasers[i].x-10, g.enemy_lasers[i].y-10, 25, 25 });
                    g.enemy_lasers[i] = g.enemy_lasers[g.enemy_lasers_count-1];
                    g.enemy_laser_lives[i] = g.enemy_laser_lives[g.enemy_lasers_count-1];
                    g.enemy_lasers_count--;
                }
            }

            // fire laser
            if (g.input.fire) {
                if(g.lasers_count < MAX_LASERS) {
                    g.lasers[g.lasers_count] = (SpaceThing) {
                      .x = g.player.thing.x+cos((g.player.thing.angle))*8,
                      .y = g.player.thing.y+sin((g.player.thing.angle))*8,
                      .angle = g.player.thing.angle,
                      .velocity_x = g.player.thing.velocity_x + cos(g.player.thing.angle) * 4,
                      .velocity_y = g.player.thing.velocity_y + sin(g.player.thing.angle) * 4,
                    };
                    g.laser_lives[g.lasers_count] = 2;
                    g.lasers_count++;
                }
            }

            //enemy fire
            if(g.enemies_count > 0) {
                rand_seed = PRINTABLE(g.t);
                if(random_float() < .05f) {
                    if(g.enemy_lasers_count < MAX_ENEMY_LASERS) {
                        int enemy_index = random() % g.enemies_count;
                        g.enemy_lasers[g.enemy_lasers_count] = g.enemies[enemy_index].thing;
                        g.enemy_laser_lives[g.enemy_lasers_count] = 2;

                        g.enemy_lasers[g.enemy_lasers_count].velocity_x = cos(g.enemies[enemy_index].thing.angle) * 4,
                        g.enemy_lasers[g.enemy_lasers_count].velocity_y = sin(g.enemies[enemy_index].thing.angle) * 4,                        
                        g.enemy_lasers_count++;
                    }
                }
            }

            //move asteroids
            for (int i = 0; i < g.asteroids_count; ++i) {
                #define ast g.asteroids[i]
                ast.thing.x = ((int)(ast.thing.x + ast.thing.velocity_x + W) % W);
                ast.thing.y = ((int)(ast.thing.y + ast.thing.velocity_y + H) % H);
                ast.thing.angle = ast.thing.angle + .02;
                #undef ast
            }


            //move lasers
            for (int i = 0; i < g.lasers_count; ++i) {
                #define laser (g.lasers[i])
                Rect old_rect = (Rect) { laser.x-10, laser.y-10, 25, 25 };
                laser.x = laser.x + laser.velocity_x;
                laser.y = laser.y + laser.velocity_y;
                if (laser.x < 0) { add_special_clear(old_rect); laser.x = W; }
                if (laser.x > W) { add_special_clear(old_rect); laser.x = 0; }
                if (laser.y < 0) { add_special_clear(old_rect); laser.y = H; }
                if (laser.y > H) { add_special_clear(old_rect); laser.y = 0; }
                #undef laser
            }

            //move enemy lasers
            for (int i = 0; i < g.enemy_lasers_count; ++i) {
                #define laser (g.enemy_lasers[i])
                laser.x = laser.x + laser.velocity_x;
                laser.y = laser.y + laser.velocity_y;
                if (laser.x < 0) laser.x = W;
                if (laser.x > W) laser.x = 0;
                if (laser.y < 0) laser.y = H;
                if (laser.y > H) laser.y = 0;
                #undef laser
            }
            // move enemies
            for (int i = 0; i < g.enemies_count; ++i)
            {
                #define enemy (g.enemies[i])
                v2 distance = (v2){ g.player.thing.x - enemy.thing.x, g.player.thing.y - enemy.thing.y };
                float theta = atan2(distance.y, distance.x);
                enemy.thing.angle = theta;
                enemy.thing.velocity_x = cos(theta);
                enemy.thing.velocity_y = sin(theta);
                enemy.thing.x = enemy.thing.x + enemy.thing.velocity_x;
                enemy.thing.y = enemy.thing.y + enemy.thing.velocity_y;
                if (enemy.thing.x < 0) enemy.thing.x = W;
                if (enemy.thing.x > W) enemy.thing.x = 0;
                if (enemy.thing.y < 0) enemy.thing.y = H;
                if (enemy.thing.y > H) enemy.thing.y = 0;
                #undef enemy        
            }

            //check laser:asteroid collisions
            for (int i = g.lasers_count - 1; i >= 0; --i) {
                for (int o = g.asteroids_count - 1; o >= 0; --o) {
                    if (collide(g.lasers[i], g.asteroids[o].thing)) {
                        add_special_clear((Rect){ g.lasers[i].x-10, g.lasers[i].y-10, 25, 25 });
                        g.lasers[i] = g.lasers[g.lasers_count - 1];
                        g.laser_lives[i] = g.laser_lives[g.lasers_count - 1];
                        g.lasers_count--;
                        g.asteroids[o].health--;
                        g.asteroids[o].thing.size--;
                        if (g.asteroids[o].health <= 0) {
                            add_special_clear((Rect)
                            {
                                g.asteroids[o].thing.x-(g.asteroids[o].thing.size+6), 
                                g.asteroids[o].thing.y-(g.asteroids[o].thing.size+6), 
                                g.asteroids[o].thing.size*2+12, 
                                g.asteroids[o].thing.size*2+12
                            });
                            g.asteroids[o] = g.asteroids[g.asteroids_count - 1];
                            g.asteroids_count--;
                        }

                        break;
                    }
                }
            }

            //check laser:enemy collisions
            for (int i = g.lasers_count - 1; i >= 0; --i) {
                for (int o = g.enemies_count - 1; o >= 0; --o) {
                    if(collide(g.lasers[i], g.enemies[o].thing)) {
                        add_special_clear((Rect){ g.lasers[i].x-10, g.lasers[i].y-10, 25, 25 });
                        g.lasers[i] = g.lasers[g.lasers_count-1];
                        g.laser_lives[i] = g.laser_lives[g.lasers_count-1];
                        g.lasers_count--;
                        g.enemies[o].health--;
                        if(g.enemies[o].health <= 0){
                            add_special_clear((Rect){ g.enemies[o].thing.x-50, g.enemies[o].thing.y-50, 100, 100 });
                            g.enemies[o] = g.enemies[g.enemies_count-1];
                            g.enemies_count--;
                        }
                    }
                }
            }

            //check player collisions
            {
                for (int i = 0; i < g.asteroids_count; ++i) {
                    if(collide(g.player.thing, g.asteroids[i].thing)) {
                        g.current_game_state = GameState_LEVELSELECT;
                    }
                }

                for (int i = 0; i < g.enemies_count; ++i) {
                    if(collide(g.player.thing, g.enemies[i].thing)) {
                        g.current_game_state = GameState_LEVELSELECT;
                    }
                }

                for (int i = g.enemy_lasers_count - 1; i >= 0; --i) {
                    if(collide(g.player.thing, g.enemy_lasers[i])) {
                        g.player.fuel -= 5;
                        add_special_clear((Rect){ g.enemy_lasers[i].x-10, g.enemy_lasers[i].y-10, 25, 25 }); 
                        g.enemy_lasers[i] = g.enemy_lasers[g.enemy_lasers_count-1];
                        g.enemy_laser_lives[i] = g.enemy_laser_lives[g.enemy_lasers_count-1];
                        g.enemy_lasers_count--;
                    }
                }

            }

            //check for victory
            {
                if(g.asteroids_count <= 0 && g.enemies_count <= 0) {
                    if(g.unlocked_levels < 4 && g.current_level == g.unlocked_levels) {
                        g.unlocked_levels++;
                    }
                    g.current_game_state = GameState_MISSIONCOMPLETE;
                    g.t = 0;
                }
            }

            g.t += TIME_STEP;
        } break;
        case GameState_MISSIONCOMPLETE: {
            g.t += TIME_STEP;
            if(g.t > 3) {
                g.current_game_state = GameState_LEVELSELECT;
            }

            float w = 350+g.t*30;
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
    bool foo = elapsed > TIME_STEP;
    while(elapsed > TIME_STEP) {
        poll_input();
        tick();
        g.prev_input = g.input;
        elapsed -= TIME_STEP;
        //printf("after tick function\n");
    }
    if(foo){
        render();
#ifdef RPI        
        swap_buffers();
#endif
    }
    last = clock();
}

void health_bar(Color2 from, Color2 to, Rect r, float max) {
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

    for(int _x = min_x; _x < max_x; _x++) {
        Color col = lerp_color(from, to, (_x - min_x) / (float)(max - min_x));
        draw_target.pixels[min_y*W+_x] = col;
    }

    for(int _y = min_y+1; _y < max_y; _y++)
    for(int _x = min_x; _x < max_x; _x++) {
        draw_target.pixels[_y*W+_x] = draw_target.pixels[(_y-1)*W+_x];
    }
}

double pow_simple(double x, double y) {
    if (x <= 0.0) {
        return 0.0; // or handle as special case
    }
    return exp(y * log(x));
}

float extreme_sine(float t) {
    float s = sin(t); // t in radians
    float a = fabs(s);
    return a * a * a * a * a * a * a * a; // octic curve -- flat near 0, steeper near 1
}

void gradient() {
    for(int _y = 0; _y < H; _y++)
    for(int _x = 0; _x < W; _x++) {
        float t = fabs(sin(_y/(float)H*3.0f));
        draw_target.pixels[_y*W+_x] = lerp_color((Color2){}, (Color2){.b = 0x55 }, t);
    }
}

void draw_ship(SpaceThing thing, Color wing_color, float thrust_level);

Texture background;
Color background_pixels[W*H];
bool background_inited;

Texture flork_texture;
Color flork_pixels[120*120];
bool flork_inited;
void render() {
    TIME_BLOCK("render",
    switch(g.current_game_state) {
        case GameState_PLAY: {
            if(!flork_inited){
                flork_texture = (Texture)
                {
                    .pixels = flork_pixels,
                    .width = 120,
                    .height = 120,
                };

                for (int i = 0; i < 120*120; ++i)
                {
                    flork_pixels[i] = random();
                }
                flork_inited = true;
            }
            if(!background_inited) {
                Color *pixels = draw_target.pixels;
                draw_target.pixels = background_pixels;
                background = (Texture){
                    .width = W, 
                    .height = H,
                    .pixels = background_pixels
                };

                gradient();
                rand_seed = 3;
                //static stars
                for (int i = 0; i < 1000; ++i)
                {
                    u32 x = round(random_float() * W);
                    u32 y = round(random_float() * H);

                    float size = random() % 2 + 1;
                    float half_size = size/2.0f;
                    fill_rect(white, (Rect){ x - half_size, y - half_size, size, size });
                }

                draw_target.pixels = pixels;
                background_inited = true;
            }

            rand_seed = 2;
            static bool twinkle_stars_inited = false;

            TIME_BLOCK("twinkle init",
                if(!twinkle_stars_inited)
                {
                    //generate twinkle stars
                    for (int i = 0; i < 50; ++i)
                    {
                        g.twinkle_stars[i] = (v2){ round(random_float() * W), round(random_float() * H) };
                    }

                    twinkle_stars_inited = true;
                }
            );

            //so both buffers get the background
            if(g.frame < 2) {
                //printf("target: %d\n", draw_target.pixels);
                draw_texture(background);
                g.frame++;
            }

            //CLEAR background rects
            {
#ifdef RPI              
                TIME_BLOCK("clear rect",
                    for (int i = 0; i < g.clear_rects_count; ++i)
                    {
                        sample_texture_region(background, g.clear_rects[i]);
                    }
                    
                    if(g.special_clear_count > 1) {
                        for (int i = 0; i < g.special_clear_count; ++i)
                        {
                            sample_texture_region(background, g.special_clear[i]);
                        }
                    }
                );

#else
                for (int i = 0; i < g.clear_rects_count; ++i)
                {
                    sample_texture_region(background, g.clear_rects[i]);
                }
#endif
            }


#ifdef _WIN32
            static float t = 0;
            TIME_BLOCK("twinkle render",
                //draw twinkle stars
                for (int i = 0; i < 50; ++i)
                {
                    float actual_t;
                    TIME_BLOCK("twinkle actual t", {
                        actual_t = (t*5) + (random_float()*1000);
                    });

                    float size;
                    TIME_BLOCK("twinkle extreme_sine", {
                        size = (extreme_sine(actual_t)*4)+1;
                    });

                    float half_size = size/2.0f;
                    fill_rect(white, (Rect){ g.twinkle_stars[i].x - half_size, g.twinkle_stars[i].y - half_size, size, size });
                }
            );

            t += .01f;
#endif            

            TIME_BLOCK("enemies render", 
                for (int i = 0; i < g.enemies_count; ++i)
                {
                    draw_ship(g.enemies[i].thing, make_color(0xBB, 0, 0, 0), .2f);
                }
            );

            TIME_BLOCK("player render",
                draw_ship(g.player.thing, make_color(0, 0xBB, 0, 0), g.player.thrust);
            );

            for (int i = 0; i < g.asteroids_count; ++i)
            {
                //printf("%d: %f\n", i, g.asteroids[i].thing.size);
                Asteroid ast = g.asteroids[i];
                TIME_BLOCK("ast circle render",
#ifdef _WIN32                    
                    fill_circle(make_color(95, 50, 0, 255), (v2){ ast.thing.x, ast.thing.y }, ast.thing.size);
#else
                    fill_rect(make_color(95, 50, 0, 255), (Rect){ ast.thing.x-ast.thing.size, ast.thing.y-ast.thing.size, ast.thing.size*2, ast.thing.size*2 });
#endif
                );
            }

            for (int i = 0; i < g.lasers_count; ++i)
            {
#ifdef _WIN32
                fill_circle(make_color(0, 0xFF, 0xFF, 0), (v2){ g.lasers[i].x, g.lasers[i].y }, 5);
#else
                fill_rect(make_color(0, 0xFF, 0xFF, 0), (Rect){ g.lasers[i].x-5, g.lasers[i].y-5, 10, 10 });
#endif
            }

            for (int i = 0; i < g.enemy_lasers_count; ++i)
            {
#ifdef _WIN32
                fill_circle(make_color(0, 0xFF, 0xFF, 0), (v2){ g.enemy_lasers[i].x, g.enemy_lasers[i].y }, 5);
#else                
                fill_rect(make_color(0, 0xFF, 0xFF, 0), (Rect){ g.enemy_lasers[i].x-5, g.enemy_lasers[i].y-5, 10, 10 });
#endif
            }

            //HUD
            {
                float padding = 30;
                float max_width = 300;
                float adjusted_width = (g.player.fuel/100.0f)*max_width;

                TIME_BLOCK("health bar",
                    float x =  W/2-max_width/2;
                    health_bar( (Color2){.r = 255 }, (Color2){.g = 255 }, (Rect){ W/2-max_width/2, H - 30, adjusted_width, 20 }, x+max_width);
                );
            }
        } break;
        default:
            break;
    }
    );
}


void draw_ship(SpaceThing thing, Color wing_color, float thrust_level){
    //fill_circle(green, (v2){ g.player.thing.x, g.player.thing.y }, g.player.thing.size);
    v2 a = (v2){-15,-20};
    v2 b = (v2){7,0};
    v2 c = (v2){-15, 20};
    a = rotate_point(a, thing.angle);
    b = rotate_point(b, thing.angle);
    c = rotate_point(c, thing.angle);
    a.x += thing.x;
    a.y += thing.y;
    b.x += thing.x;
    b.y += thing.y;
    c.x += thing.x;
    c.y += thing.y;
    fill_triangle(a, b, c, wing_color);

    a = (v2){-17,-10};
    b = (v2){20,0};
    c = (v2){-17, 10};
    a = rotate_point(a, thing.angle);
    b = rotate_point(b, thing.angle);
    c = rotate_point(c, thing.angle);
    a.x += thing.x;
    a.y += thing.y;
    b.x += thing.x;
    b.y += thing.y;
    c.x += thing.x;
    c.y += thing.y;
    fill_triangle(a, b, c, make_color(0xDD, 0xDD, 0xDD, 0));

    //thruster
    if(thrust_level) {
        a = (v2){-17,-7 };
        b = (v2){-(18+thrust_level*(20+sin(g.t*30)*4)),0};
        c = (v2){-17, 7 };
        a = rotate_point(a, thing.angle);
        b = rotate_point(b, thing.angle);
        c = rotate_point(c, thing.angle);
        a.x += thing.x;
        a.y += thing.y;
        b.x += thing.x;
        b.y += thing.y;
        c.x += thing.x;
        c.y += thing.y;            
        fill_triangle(a, b, c, red);
    }    
}