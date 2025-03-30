#include "hitbox.hpp"
#include <raylib.h>
#include <raymath.h>
#include <string>

int main(void) {
    InitWindow(0, 0, "Sigma sigma on the wall");

    shapes::Polygon poly1((Vector2){450, 350},
                          {(Vector2){400, 300}, (Vector2){500, 300}, (Vector2){500, 400}, (Vector2){400, 400}});
    // shapes::Polygon poly2((Vector2){ 700, 400 }, { (Vector2){ 700, 300 }, (Vector2){ 600, 400 }, (Vector2){ 700, 500
    // }, (Vector2){ 800, 400 } }); shapes::Circle poly1((Vector2){ 500.0f, 500.0f }, 50.0f);
    shapes::Circle poly2((Vector2){300.0f, 300.0f}, 100.0f);

    shapes::Polygon* poly_control = &poly1;
    int controlling = 1;

    /*Vector2 uncollide = Vector2Zero();*/
    while (!WindowShouldClose()) {
        Vector2 movement{};

        if (IsKeyPressed(KEY_A)) {
            movement.x = -10.0f;
        } else if (IsKeyPressed(KEY_D)) {
            movement.x = 10.0f;
        } else if (IsKeyPressed(KEY_W)) {
            movement.y = -10.0f;
        } else if (IsKeyPressed(KEY_S)) {
            movement.y = 10.0f;
            // } else if (IsKeyPressed(KEY_SPACE)) {
            //     if (controlling == 1) {
            //         controlling = 2;
            //         poly_control = &poly2;
            //     } else {
            //         controlling = 1;
            //         poly_control = &poly1;
            //     }
        } else if (IsKeyPressed(KEY_LEFT)) {
            poly_control->rotate(-5.0f);
        } else if (IsKeyPressed(KEY_RIGHT)) {
            poly_control->rotate(5.0f);
        }
        poly_control->translate(movement);

        BeginDrawing();
        ClearBackground(WHITE);

        DrawText(("COLLISION: " + std::to_string(check_collision(poly1, poly2))).c_str(), 10, 10, 20, BLACK);
        DrawText(("CONTROLLING: " + std::to_string(controlling)).c_str(), 10, 30, 20, BLACK);

        poly1.draw_lines_2D(BLACK);
        // poly2.draw_lines_2D(BLACK);
        // DrawCircleV(poly1.center, poly1.radius, BLACK);
        DrawCircleV(poly2.center, poly2.radius, BLACK);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
