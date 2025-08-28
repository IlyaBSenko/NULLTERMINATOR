// NULL TERMINATOR — Controls Sandbox (C + raylib)
// Aim with mouse/trackpad, custom crosshair, click to "fire".
// No enemies yet—just visual feedback for input.
// mac build: gcc controls_sandbox.c -o null_controls -O2 -Wall -std=c99 -lraylib -lm

#include "raylib.h" // library for game functions
#include <math.h>   

// screen size constants
#define SCREEN_W 960
#define SCREEN_H 540

// struct to store shot effect (start point, end point and time to live)
typedef struct {
    Vector2 a, b;     // line from A (player) to B (impact point = cursor)
    float   life;     // remaining lifetime (seconds)
} ShotTrace;

#define MAX_TRACES 128

static ShotTrace traces[MAX_TRACES];  
static int traceCount = 0;            // how many are alive in array


// helper function to add traces
static void AddTrace(Vector2 a, Vector2 b) {
    if (traceCount >= MAX_TRACES) return;
    traces[traceCount++] = (ShotTrace){ a, b, 0.12f }; // short-lived flash
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

// function to draw crosshair (custom)
static void DrawCrosshair(Vector2 p) {
    // simple 1-bit crosshair (no system cursor)
    const int arm = 8;
    const int gap = 4;
    DrawLineEx((Vector2){p.x - (gap+arm), p.y}, (Vector2){p.x - gap, p.y}, 2.0f, WHITE);
    DrawLineEx((Vector2){p.x + gap, p.y}, (Vector2){p.x + (gap+arm), p.y}, 2.0f, WHITE);
    DrawLineEx((Vector2){p.x, p.y - (gap+arm)}, (Vector2){p.x, p.y - gap}, 2.0f, WHITE);
    DrawLineEx((Vector2){p.x, p.y + gap}, (Vector2){p.x, p.y + (gap+arm)}, 2.0f, WHITE);
    DrawCircleV(p, 1.5f, WHITE);
}

/**
 * request anti aliasing and vsync before opening window
 */
int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT); 
    InitWindow(SCREEN_W, SCREEN_H, "NULL TERMINATOR — Controls Sandbox");
    SetTargetFPS(60);

    // lock the “player” at center for now
    Vector2 player = { SCREEN_W * 0.5f, SCREEN_H * 0.5f };

    // hide the OS cursor; we’ll draw our own crosshair
    HideCursor();

    // minimal screen shake on click (for feel)
    float shakeTime = 0.0f;
    float shakeMag  = 4.0f;

    // game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // input
        Vector2 mouse = GetMousePosition();

        bool fired = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE);
        if (fired) {
            AddTrace(player, mouse);
            shakeTime = 0.06f; // tiny bump of juice
        }

        // update
        UpdateTraces(dt);
        if (shakeTime > 0.0f) shakeTime -= dt;

        // camera shake offset (just for flavor)
        Vector2 cam = {0};
        if (shakeTime > 0.0f) {
            cam.x = (GetRandomValue(-100, 100) / 100.0f) * shakeMag;
            cam.y = (GetRandomValue(-100, 100) / 100.0f) * shakeMag;
        }

        // draw
        BeginDrawing();
            ClearBackground(BLACK);

            // arena bounds (we’ll replace later)
            // Rectangle arena = { 24 + cam.x, 24 + cam.y, SCREEN_W - 48, SCREEN_H - 48 };
            // DrawRectangleLinesEx(arena, 2.0f, WHITE);

            // draw traces
            for (int i = 0; i < traceCount; ++i) {
                float t = traces[i].life / 0.12f; // 1→0
                float thickness = 3.0f * t + 1.0f; // start thicker
                // apply camera offset
                Vector2 A = (Vector2){ traces[i].a.x + cam.x, traces[i].a.y + cam.y };
                Vector2 B = (Vector2){ traces[i].b.x + cam.x, traces[i].b.y + cam.y };
                DrawLineEx(A, B, thickness, WHITE);
                // small muzzle flash
                DrawCircleV(A, 4.0f * t + 1.0f, WHITE);
            }

            // player (center)
            DrawCircleV((Vector2){player.x + cam.x, player.y + cam.y}, 10.0f, WHITE);
            DrawCircleV((Vector2){player.x + cam.x, player.y + cam.y}, 8.0f, BLACK); // 1-bit ring look

            // draw crosshair at actual mouse (no shake on UI)
            DrawCrosshair(mouse);

            // HUD
            DrawText("Aim with trackpad/mouse. Click (or SPACE) to fire. Press ESC to quit.", 16, 12, 18, WHITE);

        EndDrawing();
    }

    ShowCursor();
    CloseWindow();
    return 0;
}

