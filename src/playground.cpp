#include "effects.hpp"
#include "particle_system.hpp"

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

    auto system = effect::Plosion{
        .type = effect::Plosion::Im,
        .radius = 10.0f,
        .particle_count = 350,
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
            system.update(static_cast<float>(delta_time));

            if (system.particles.alive_count == 0) start = false;
        }
    }

    CloseWindow();

    return 0;
}

/*#include "quadtree.hpp"*/
/**/
/*#include <cstdint>*/
/*#include <raylib.h>*/
/*#include <raymath.h>*/
/**/
/*int main(void) {*/
/*    InitWindow(0, 0, "skibidi");*/
/**/
/*    quadtree::QuadTree<3, quadtree::pos<uint8_t>> qt(*/
/*        quadtree::Box({0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()}));*/
/**/
/*    double prev_time = 0.;*/
/*    while (!WindowShouldClose()) {*/
/*        double current_time = GetTime();*/
/*        double delta_time = current_time - prev_time;*/
/*        prev_time = current_time;*/
/**/
/*        PollInputEvents();*/
/*        BeginDrawing();*/
/**/
/*        ClearBackground(WHITE);*/
/*        for (const auto& p : qt.data) {*/
/*            auto pos = p.val.position();*/
/*            DrawCircleV(pos, 20.0f, BLACK);*/
/*            DrawText(std::format("{}", p.id).c_str(), pos.x, pos.y, 5, WHITE);*/
/*        }*/
/*        qt.draw_bbs(RED);*/
/**/
/*        EndDrawing();*/
/*        SwapScreenBuffer();*/
/**/
/*        if (IsKeyPressed(KEY_SPACE)) {*/
/*            auto pos = GetMousePosition();*/
/*            qt.insert(quadtree::pos(pos, (uint8_t)0));*/
/*        } else if (IsKeyPressed(KEY_D)) {*/
/*            for (std::size_t i = 0; i < qt.data.size(); i++) {*/
/*                if (CheckCollisionPointCircle(GetMousePosition(), qt.data[i].val.position(), 20.0f)) {*/
/*                    qt.remove(i);*/
/*                    break;*/
/*                }*/
/*            }*/
/*        } else if (IsKeyPressed(KEY_P)) {*/
/*            qt.prune();*/
/*        } else if (IsKeyPressed(KEY_A)) {*/
/*            qt.print([](auto& x, auto y) {*/
/**/
/*            });*/
/*        }*/
/*    }*/
/**/
/*    CloseWindow();*/
/**/
/*    return 0;*/
/*}*/
