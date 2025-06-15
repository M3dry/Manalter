#include "effects.hpp"
#include "particle_system.hpp"

#include <raylib.h>
#include <raymath.h>

#include <print>

int main(void) {
    InitWindow(0, 0, "particle system");

    Camera3D cam{};
    cam.position = {0.0f, 10.0f, 30.0f};
    cam.target = Vector3Zero();
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 90.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    auto system = effect::Plosion{
        .type = effect::Plosion::Ex,
        .radius = 0.0f,
        .particle_count = 35000,
        .particle_size_scale = 0.05f,
        .floor_y = 0.0f,
        .lifetime = {0.5f, 1.0f},
        .velocity_scale = {5.0f, 55.0f},
        .acceleration = {50.0f, 50.0f, 50.0f},
        .color = {{WHITE, 10.0f}, {BLUE, 80.0f}},
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
        DrawSphere(Vector3Zero(), 0.5f, RED);
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
        }

        if (start) {
            if (!system.update(static_cast<float>(delta_time))) {
                start = false;
                system.reset();
            }
        }
    }

    CloseWindow();

    return 0;
}
