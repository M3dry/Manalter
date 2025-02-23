#include "hitbox.hpp"
#include "input.hpp"
#include "ui.hpp"

#include <format>
#include <raylib.h>
#include <raymath.h>

int main(void) {
    InitWindow(0, 0, "Sigma sigma on the wall");

    ui::Draggable draggable_square({30.0f, 30.0f}, (Rectangle){30.0f, 30.0f, 200.0f, 50.0f}, [](auto origin) {
        DrawRectangle(origin.x, origin.y, 200.0f, 50.0f, BLACK);
    });
    Mouse mouse;
    Rectangle dropoff = {
        .x = 500.0f,
        .y = 500.0f,
        .width = 50.0f,
        .height = 50.0f,
    };
    shapes::Polygon dropoff_poly = dropoff;
    int dropped_counter = 0;

    while (!WindowShouldClose()) {
        mouse.poll();

        BeginDrawing();
        ClearBackground(WHITE);

        DrawRectangleRec(dropoff, RED);
        DrawText(std::format("Dropped: {}", dropped_counter).c_str(), 10, 10, 20, BLACK);

        if (auto dropped_at = draggable_square.update(mouse); dropped_at && check_collision(dropoff_poly, *dropped_at)) {
            dropped_counter++;
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
