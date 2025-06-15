#include "quadtree.hpp"

#include <cstdint>
#include <raylib.h>
#include <raymath.h>

int main(void) {
    InitWindow(0, 0, "quadtree");

    quadtree::QuadTree<4, quadtree::pos<uint8_t>> qt(
        quadtree::Box({0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()}));

    double prev_time = 0.;
    while (!WindowShouldClose()) {
        double current_time = GetTime();
        double delta_time = current_time - prev_time;
        prev_time = current_time;

        PollInputEvents();
        BeginDrawing();

        ClearBackground(WHITE);
        for (const auto& p : *qt.data) {
            auto pos = p.val.position();
            DrawCircleV(pos, 20.0f, BLACK);
            DrawText(std::format("{}", p.id).c_str(), pos.x, pos.y, 5, WHITE);
        }
        qt.draw_bbs(RED, true);

        EndDrawing();
        SwapScreenBuffer();

        if (IsKeyPressed(KEY_SPACE)) {
            auto pos = GetMousePosition();
            qt.insert(quadtree::pos(pos, (uint8_t)0));
        } else if (IsKeyPressed(KEY_D)) {
            for (std::size_t i = 0; i < qt.data->size(); i++) {
                if (CheckCollisionPointCircle(GetMousePosition(), (*qt.data)[i].val.position(), 20.0f)) {
                    qt.remove(i);
                    break;
                }
            }
        } else if (IsKeyPressed(KEY_P)) {
            qt.rebuild();
        } else if (IsKeyPressed(KEY_A)) {
            qt.print([](auto& x, auto y) {

            });
        }
    }

    CloseWindow();

    return 0;
}
