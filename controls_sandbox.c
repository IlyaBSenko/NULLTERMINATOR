// NULL TERMINATOR — Controls Sandbox (C + raylib)
// Aim with mouse/trackpad, custom crosshair, click to "fire".
// mac build: gcc controls_sandbox.c -o null_controls -O2 -Wall -std=c99 -lraylib -lm

#include "raylib.h" // library for game functions
#include <math.h>   

// screen
#define SCREEN_W 960
#define SCREEN_H 540

// starter pistol
#define FIRE_RATE 6.0f 
#define TRACE_LIFE 0.12f

// starter pistol bullets
#define BULLET_SPEED 540.0f // pixels per second
#define BULLET_LIFETIME 0.6f // bullet on screen time
#define BULLET_RADIUS 3.0f // how big it is
#define MAX_BULLETS 256 

// #define ENEMY_SPEED 85.0f // pixels/sec
#define ENEMY_RADIUS 8.0f
#define MAX_ENEMIES 256
#define ENEMY_SPAWN_INTERVAL 1.0f // spawn one per sec (fix later)

#define PLAYER_RADIUS 10.0f   // was hardcoded in draw; now a constant
#define HP_MAX 6              // 3 hearts × 2 hits each
#define HEARTS 3
#define HIT_IFRAME 0.8f       // seconds of invulnerability after a hit

// hearts UI layout
#define HEART_SIZE 18.0f      // logical size for drawing
#define HEART_GAP  10         // pixels between hearts

// dificulty ramp (time based)
#define SPAWN_BASE 1.00f // seconds between spawns at t=0
#define SPAWN_MIN 0.20f // fastest allowed (for now heeheehee)
#define SPAWN_RAMP 0.015f // how much to subract per second on linear approach

// difficulty ramper
#define ENEMY_SPEED_BASE 85.0f // starting speed, was ENEMY_SPEED
#define ENEMY_SPEED_MAX 220.f // limit, no cap aye
#define ENEMY_SPEED_RAMP 0.60f // +speed per second, 36 per minute bruh

// shotgun stats
#define SHOTGUN_UNLOCK_AFTER_SCORE 500
#define SHOTGUN_PELLETS 5
#define SHOTGUN_SPREAD_DEG 18.0f // around 18 degreees spread each slide
#define SHOTGUN_FIRE_RATE 2.8f



// struct to store shot effect (start point, end point and time to live)
typedef struct {
    Vector2 a, b;     // line from A (player) to B (impact point = cursor)
    float   life;     // remaining lifetime (seconds)
} ShotTrace;


typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    int alive;
} Bullet;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int alive;
} Enemy;

typedef enum { STATE_PLAYING = 0, STATE_GAME_OVER = 1 } GameState;

static Enemy enemies[MAX_ENEMIES];
static int enemyCount = 0;

static Bullet bullets[MAX_BULLETS];
static int bulletCount = 0;

#define MAX_TRACES 128

static ShotTrace traces[MAX_TRACES];  
static int traceCount = 0;            // how many are alive in array


// helper function to add traces
// helper function to add traces
static void AddTrace(Vector2 a, Vector2 b) {
    if (traceCount >= MAX_TRACES) return;
    traces[traceCount++] = (ShotTrace){ a, b, TRACE_LIFE };  // short-lived flash
}


// helper to update and remove expired traces
static void UpdateTraces(float dt) {
    for (int i = traceCount - 1; i >= 0; --i) {
        traces[i].life -= dt;
        if (traces[i].life <= 0.0f) {
            // remove by swap
            traces[i] = traces[traceCount - 1];
            traceCount--;
        }
    }
}

static void AddBullet(Vector2 from, Vector2 to) {
    if (bulletCount >= MAX_BULLETS) return;

    // direction = normalized (to > from)
    Vector2 dir = { to.x - from.x, to.y - from.y };
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
    if (len <= 0.0001f) return;
    dir.x /= len; dir.y /= len;

    bullets[bulletCount++] = (Bullet) {
        .pos = from,
        .vel = (Vector2){ dir.x * BULLET_SPEED, dir.y * BULLET_SPEED },
        .life = BULLET_LIFETIME,
        .alive = 1
    };
}

static void UpdateBullets(float dt) {
    for (int i = bulletCount - 1; i>= 0; --i) {
        Bullet *b = &bullets[i];
        if (!b->alive) continue;

        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;
        b->life   -= dt;

        // kill if expired or off-screen
        if (b->life <= 0.0f ||
            b->pos.x < -20 || b->pos.x > SCREEN_W + 20 ||
            b->pos.y < -20 || b->pos.y > SCREEN_H + 20) {
                bullets[i] = bullets[bulletCount - 1];
                bulletCount--;
        }
    }
}

static void DrawCrosshair(Vector2 p) {
    const int arm = 8;
    const int gap = 4;
    DrawLineEx((Vector2){p.x - (gap+arm), p.y}, (Vector2){p.x - gap, p.y}, 2.0f, WHITE);
    DrawLineEx((Vector2){p.x + gap, p.y}, (Vector2){p.x + (gap+arm), p.y}, 2.0f, WHITE);
    DrawLineEx((Vector2){p.x, p.y - (gap+arm)}, (Vector2){p.x, p.y - gap}, 2.0f, WHITE);
    DrawLineEx((Vector2){p.x, p.y + gap}, (Vector2){p.x, p.y + (gap+arm)}, 2.0f, WHITE);
    DrawCircleV(p, 1.5f, WHITE);
}

static void AddEnemy(Vector2 player, float speed) {
    if (enemyCount >= MAX_ENEMIES) return;

    int side = GetRandomValue(0, 3);
    Vector2 p = {0};

    switch (side) {
        case 0: p.x = -10;               p.y = GetRandomValue(0, SCREEN_H); break;
        case 1: p.x = SCREEN_W + 10;     p.y = GetRandomValue(0, SCREEN_H); break;
        case 2: p.x = GetRandomValue(0, SCREEN_W); p.y = -10;               break;
        case 3: p.x = GetRandomValue(0, SCREEN_W); p.y = SCREEN_H + 10;     break;
    }

    Vector2 dir = (Vector2){ player.x - p.x, player.y - p.y };
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
    if (len < 0.0001f) len = 0.0001f;
    dir.x /= len; dir.y /= len;

    enemies[enemyCount++] = (Enemy){
        .pos = p,
        .vel = (Vector2){ dir.x * speed, dir.y * speed },  
        .alive = 1
    };
}


static void UpdateEnemies(float dt) {
    for (int i = enemyCount - 1; i >= 0; --i) {
        Enemy *e = &enemies[i];
        if (!e->alive) continue;

        e->pos.x += e->vel.x * dt;
        e->pos.y += e->vel.y * dt;

        if (e->pos.x < -50 || e->pos.x > SCREEN_W + 50 ||
            e->pos.y < -50 || e->pos.y > SCREEN_H + 50) {
            enemies[i] = enemies[enemyCount - 1];
            enemyCount--;
        }
    }
}

static float Dist2(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx*dx + dy*dy;
}

// HEARTS / FIX LATER

// Filled heart: two lobes + wide body triangle + small V-notch at top.
static void DrawHeartFilled(Vector2 p, float s) {
    // Tweakable proportions that read well at small sizes (16–24px)
    float r   = s * 0.34f;                 // lobe radius
    float lift= s * 0.08f;                 // raise the seam slightly
    float body= s * 0.95f;                 // bottom point depth

    // Lobe centers slightly above centerline
    Vector2 left   = (Vector2){ p.x - r, p.y - lift };
    Vector2 right  = (Vector2){ p.x + r, p.y - lift };
    Vector2 bottom = (Vector2){ p.x,     p.y + body };

    // Merge lobes
    DrawCircleV(left,  r, WHITE);
    DrawCircleV(right, r, WHITE);

    // Body triangle (wider than lobe distance so sides look rounded)
    Vector2 tl = (Vector2){ p.x - 2.15f*r, p.y };
    Vector2 tr = (Vector2){ p.x + 2.15f*r, p.y };
    DrawTriangle(tl, tr, bottom, WHITE);

    // Carve a small V-notch (triangle) so it reads as a heart, not a blob
    Vector2 vA = (Vector2){ p.x,            p.y - r*0.25f };
    Vector2 vB = (Vector2){ p.x - r*0.70f,  p.y + r*0.10f };
    Vector2 vC = (Vector2){ p.x + r*0.70f,  p.y + r*0.10f };
    DrawTriangle(vA, vB, vC, BLACK);
}

// Thin zig-zag crack that stays readable at small sizes.
static void DrawHeartCrack(Vector2 p, float s) {
    Vector2 a = (Vector2){ p.x,            p.y - s*0.06f };
    Vector2 b = (Vector2){ p.x - s*0.16f,  p.y + s*0.22f };
    Vector2 c = (Vector2){ p.x + s*0.10f,  p.y + s*0.52f };
    DrawLineEx(a, b, 2.0f, BLACK);
    DrawLineEx(b, c, 2.0f, BLACK);
}

// state: 0=full, 1=cracked (show crack), 2=broken (draw nothing)
static void DrawHeartIcon(Vector2 p, float s, int state) {
    if (state == 2) return;     // vanish on "broken"
    DrawHeartFilled(p, s);
    if (state == 1) DrawHeartCrack(p, s);
}




// Returns 0=full, 1=cracked, 2=broken for heart index i (0..HEARTS-1)
static int HeartStateFromHP(int hp, int i) {
    // Heart 0 covers HP 6..5, heart 1 covers 4..3, heart 2 covers 2..1
    int value = hp - (HP_MAX - (i + 1) * 2); // maps to {<=0,1,>=2}
    if (value >= 2) return 0;  // full
    if (value == 1) return 1;  // cracked
    return 2;                  // broken
}

// shotgun helper
static void FireShotgun(Vector2 from, Vector2 to) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float base = atan2f(dy, dx);

    float spread = SHOTGUN_SPREAD_DEG * (PI/180.0f);
    int n = SHOTGUN_PELLETS;

    for (int i = 0; i <n; ++i) {
        float t = (n == 1) ? 0.0f : (float)i/(float)(n-1);      // 0..1
        float ang = base + (t - 0.5f) * 2.0f * spread;          // center→edges
        Vector2 dirPoint = (Vector2){ from.x + cosf(ang), from.y + sinf(ang) };
        AddBullet(from, dirPoint);                              // your AddBullet normalizes
    }

    // draw one bright center trace for feedback
    Vector2 centerPoint = (Vector2){ from.x + cosf(base), from.y + sinf(base) };
    AddTrace(from, centerPoint);
    
}


/**
 * request anti aliasing and vsync before opening window
 */
int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT); 
    InitWindow(SCREEN_W, SCREEN_H, "NULL TERMINATOR — Controls Sandbox");
    SetTargetFPS(60);

    // lock the “player” at center for now, will upgrade later
    Vector2 player = { SCREEN_W * 0.5f, SCREEN_H * 0.5f };

    HideCursor();

    int score = 0;
    float timeSinceStart = 0.0f;

    // minimal screen shake on click (for vibbbeeessss)
    float shakeTime = 0.0f;
    float shakeMag  = 4.0f;

    float fireCooldown = 0.0f; 

    float spawnTimer = 0.0f;

    int hp = HP_MAX;
    float hurtTimer = 0;

    GameState state = STATE_PLAYING;

    bool hasShotgun = false;
    bool justUnlockedShotgun = false;
    float shotgunBannerTimer = 0.0f;

    // game loop
    while (!WindowShouldClose()) {

        float dt = GetFrameTime();
        Vector2 mouse = GetMousePosition();

        if (state == STATE_PLAYING) {
            // cooldown tick
            if (fireCooldown > 0.0f) fireCooldown -= dt;

            // upgrade unlock
            if (!hasShotgun && score >= SHOTGUN_UNLOCK_AFTER_SCORE) {
                hasShotgun = true;
                justUnlockedShotgun = true;
                shotgunBannerTimer  = 2;   // show for ~1.5 seconds
                shakeTime = 0.08f; // tiny feedback bump 
            }

            if (shotgunBannerTimer > 0.0f) shotgunBannerTimer -= dt;


            bool wantsFire = IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyDown(KEY_SPACE);
            if (wantsFire && fireCooldown <= 0.0f) {
                if (hasShotgun) {
                    FireShotgun(player, mouse);
                    fireCooldown = 1.0f / SHOTGUN_FIRE_RATE;
                } else {
                    AddTrace(player, mouse);
                    AddBullet(player, mouse);
                    fireCooldown = 1.0f / FIRE_RATE;
                }
                shakeTime = 0.06f;
            }


            // updates
            UpdateTraces(dt);
            UpdateBullets(dt);
            if (shakeTime > 0.0f) shakeTime -= dt;

            timeSinceStart += dt;

            // spawn gets faster over time (linear, clamped)
            float currentSpawnInterval = SPAWN_BASE - SPAWN_RAMP * timeSinceStart;
            if (currentSpawnInterval < SPAWN_MIN) currentSpawnInterval = SPAWN_MIN;

            // enemies get faster over time (linear, clamped)
            float currentEnemySpeed = ENEMY_SPEED_BASE + ENEMY_SPEED_RAMP * timeSinceStart;
            if (currentEnemySpeed > ENEMY_SPEED_MAX) currentEnemySpeed = ENEMY_SPEED_MAX;


            // spawn
            spawnTimer -= dt;
            if (spawnTimer <= 0.0f) {
                AddEnemy(player, currentEnemySpeed);
                spawnTimer = currentSpawnInterval;
            }

            // bullet to enemy
            for (int ei = enemyCount - 1; ei >= 0; --ei) {
                Enemy *e = &enemies[ei];
                float killRadius = ENEMY_RADIUS + BULLET_RADIUS;
                for (int bi = bulletCount - 1; bi >= 0; --bi) {
                    Bullet *b = &bullets[bi];
                    if (Dist2(e->pos, b->pos) <= killRadius * killRadius) {
                        enemies[ei] = enemies[enemyCount - 1]; enemyCount--;
                        bullets[bi] = bullets[bulletCount - 1]; bulletCount--;
                        shakeTime = 0.06f; score += 10;
                        break;
                    }
                }
            }

            // enemy to player
            if (hurtTimer > 0.0f) hurtTimer -= dt;
            for (int ei = enemyCount - 1; ei >= 0; --ei) {
                Enemy *e = &enemies[ei];
                float touchRadius = ENEMY_RADIUS + PLAYER_RADIUS;
                if (Dist2(e->pos, player) <= touchRadius * touchRadius) {
                    if (hurtTimer <= 0.0f) {
                        if (hp > 0) hp -= 1;
                        if (hp <= 0) { state = STATE_GAME_OVER; }
                        hurtTimer = HIT_IFRAME;
                        shakeTime = 0.12f;
                    }
                    enemies[ei] = enemies[enemyCount - 1]; enemyCount--;
                }
            }

            UpdateEnemies(dt);

        // STATE_GAME_OVER
        } else { 
            if (IsKeyPressed(KEY_R)) {
                // reset run
                score = 0;
                hp = HP_MAX;
                hurtTimer = 0.0f;
                shakeTime = 0.0f;
                fireCooldown = 0.0f;
                spawnTimer = 0.0f;
                timeSinceStart = 0.0f;

                enemyCount = 0;
                bulletCount = 0;
                traceCount  = 0;
                hasShotgun = false;
                justUnlockedShotgun = false;
                shotgunBannerTimer = 0.0f;

                state = STATE_PLAYING;   
            }
        }


        // camera shake offset (no real reason, just tuff)
        Vector2 cam = {0};
        if (shakeTime > 0.0f) {
            cam.x = (GetRandomValue(-100, 100) / 100.0f) * shakeMag;
            cam.y = (GetRandomValue(-100, 100) / 100.0f) * shakeMag;
        }

        // draw
        BeginDrawing();
            ClearBackground(BLACK);

            // draw traces
            for (int i = 0; i < traceCount; ++i) {
                float t = traces[i].life / TRACE_LIFE; // 1 to 0
                float thickness = 3.0f * t + 1.0f; // start thicker, pause
                // apply camera offset
                Vector2 A = (Vector2){ traces[i].a.x + cam.x, traces[i].a.y + cam.y };
                Vector2 B = (Vector2){ traces[i].b.x + cam.x, traces[i].b.y + cam.y };
                DrawLineEx(A, B, thickness, WHITE);
                // small muzzle flash
                DrawCircleV(A, 4.0f * t + 1.0f, WHITE);
            }

            for (int i = 0; i < bulletCount; ++i) {
                Vector2 p = { bullets[i].pos.x + cam.x, bullets[i].pos.y + cam.y };
                DrawCircleV(p, BULLET_RADIUS, WHITE);
            }

            // draw enemies with camera shake
            for (int i = 0; i < enemyCount; ++i) {
                Vector2 p = { enemies[i].pos.x + cam.x, enemies[i].pos.y + cam.y };
                DrawCircleV(p, ENEMY_RADIUS, WHITE);
            }

            

            // HEARTS TOP RIGHT (FIX LATER)
            const char *scoreText = TextFormat("Score: %d", score);
            int fontSize = 18;
            int scoreWidth = MeasureText(scoreText, fontSize);
            int scoreX = SCREEN_W - scoreWidth - 16;
            int scoreY = 12;

            DrawText(scoreText, scoreX, scoreY, fontSize, WHITE);

            // hearts row, aligned to the right edge under the score
            int heartsY = scoreY + fontSize + 6;
            float s = HEART_SIZE;
            for (int i = 0; i < HEARTS; ++i) {
                // draw from right to left so the row hugs right margin
                int idx = HEARTS - 1 - i;  // 2,1,0
                int state = HeartStateFromHP(hp, idx);

                float xRight = SCREEN_W - 16 - (i * (s + HEART_GAP));
                Vector2 center = (Vector2){ xRight - s*0.5f, heartsY + s*0.4f };
                DrawHeartIcon(center, s, state);
            }

            DrawText("Aim with mouse. Click to fire. ESC=Quit.", 16, 12, 18, WHITE);

            // Upgrade banner 
            if (shotgunBannerTimer > 0.0f || justUnlockedShotgun) {
            const float duration = 1.5f;
            float a = shotgunBannerTimer / duration;   // 0..1 ideally
            if (a < 0.0f) a = 0.0f;
            if (a > 1.0f) a = 1.0f;

            // simple ease: fade in first 20%, hold, fade out last 20%
            float alpha = (a < 0.2f) ? (a / 0.2f)
                    : (a > 0.8f) ? ((1.0f - a) / 0.2f)
                    : 1.0f;

            const char *msg = "SHOTGUN UNLOCKED";
            int fs = 28;
            int w = MeasureText(msg, fs);
            DrawText(msg, (SCREEN_W - w)/2, 80, fs, Fade(WHITE, alpha));

            if (shotgunBannerTimer <= 0.0f) justUnlockedShotgun = false;
        }



            // player (center) — flash while hurt
            Color playerColor = (hurtTimer > 0.0f && ((int)(hurtTimer * 20) % 2 == 0)) ? BLACK : WHITE;
            DrawCircleV((Vector2){player.x + cam.x, player.y + cam.y}, PLAYER_RADIUS, playerColor);
            DrawCircleV((Vector2){player.x + cam.x, player.y + cam.y}, PLAYER_RADIUS - 2, BLACK);

            DrawCrosshair(mouse);

            if (state == STATE_GAME_OVER) {
            // OPTIONAL MAY REMOVE LATER - dim the screen a bit 
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.35f));

            // main message
            const char *title = "PROCESS TERMINATED";
            int titleSize = 36;
            int titleW = MeasureText(title, titleSize);
            DrawText(title, (SCREEN_W - titleW)/2, SCREEN_H/2 - 40, titleSize, WHITE);

            // subtext with score + prompt
            const char *sub = TextFormat("Score: %d   -   Press R to restart", score);
            int subSize = 20;
            int subW = MeasureText(sub, subSize);
            DrawText(sub, (SCREEN_W - subW)/2, SCREEN_H/2 + 6, subSize, WHITE);
        }


            
        EndDrawing();
    }

    ShowCursor();
    CloseWindow();
    return 0;
}

