#include "effects.hpp"
#include "particle_system.hpp"
#include "utility.hpp"

#include <raylib.h>
#include <raymath.h>

int main(void) {
    InitWindow(0, 0, "skibidi");

    Camera3D cam{};
    cam.position = {0.0f, 10.0f, 30.0f};
    cam.target = Vector3Zero();
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 90.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    auto system = effect::Implosion{
        .radius = 10.0f,
        .particle_count = 5000,
        .floor_y = 0.0f,
        .lifetime = { 0.5f, 1.0f},
        .velocity_scale = { 5.0f, 15.0f },
        .acceleration = {20.0f, 20.0f, 20.0f },
        .color = { {PURPLE, 10.0f}, { BLACK, 20.0f} },
    }({0.0f, 0.0f});

    bool start = false;
    double prev_time = 0.;
    while (!WindowShouldClose()) {
        double current_time = GetTime();
        double delta_time = current_time - prev_time;
        prev_time = current_time;

        PollInputEvents();
        BeginDrawing();
        ClearBackground(DARKGRAY);
        BeginMode3D(cam);
        DrawSphere(Vector3Zero(), 1.0f, RED);
        DrawPlane(Vector3Zero(), {15000.0f, 15000.0f}, DARKGRAY);
        DrawCircle3D(Vector3Zero(), 10.0f, {1.0f, 0.0f, 0.0f}, 90.0f, WHITE);
        DrawCircle3D(Vector3Zero(), 5.0f, {1.0f, 0.0f, 0.0f}, 90.0f, WHITE);
        EndMode3D();

        BeginMode3D(cam);
        system.draw();
        EndMode3D();

        if (start) DrawText("STARTED", 10, 10, 20, WHITE);
        EndDrawing();
        SwapScreenBuffer();

        if (!start && IsKeyPressed(KEY_SPACE)) {
            start = true;

            system.reset();
        }

        if (start) {
            system.update(delta_time);

            if (system.particles.alive_count == 0) start = false;
        }
    }

    CloseWindow();

    return 0;
}
