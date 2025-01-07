#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include <string>

#define WIDTH 50.0f

#define DEG(rad) (rad * 180.0f / std::numbers::pi)

Vector2 c(float alpha) {
    return (Vector2){WIDTH / 2.0f * std::sin(alpha), WIDTH / 2.0f * std::cos(alpha)};
}

int main(int argc, char** argv) {
    InitWindow(0, 0, "Playground");
    int width = GetScreenWidth(), height = GetScreenHeight();

    Vector2 center = (Vector2){width / 2.0f, height / 2.0f};
    Vector2 mouse_pos = GetMousePosition();
    Vector2 relative, a, b;
    float angle = 0.0f;
    while (!WindowShouldClose()) {
        mouse_pos = GetMousePosition();
        relative = (Vector2){mouse_pos.x - center.x, center.y - mouse_pos.y};
        angle = std::atan(relative.y / relative.x);

        a =  c(-angle);
        b =  c(std::numbers::pi - angle);
        a = (Vector2){center.x + a.x, center.y - a.y};
        b = (Vector2){center.x + b.x, center.y - b.y};

        BeginDrawing();
        ClearBackground(WHITE);

        DrawText(("Angle: " + std::to_string(DEG(angle))).c_str(), 10, 10, 20, BLACK);
        DrawText(("COORD: [" + std::to_string(relative.x) + ";" + std::to_string(relative.y) + "]").c_str(), 10, 30, 20,
                 BLACK);
        DrawText(("MOUSE_POS: [" + std::to_string(mouse_pos.x) + ";" + std::to_string(mouse_pos.y) + "]").c_str(), 10,
                 50, 20, BLACK);
        DrawText(("CENTER: [" + std::to_string(center.x) + ";" + std::to_string(center.y) + "]").c_str(), 10, 70, 20,
                 BLACK);
        DrawText(("A: [" + std::to_string(a.x) + ";" + std::to_string(a.y) + "]").c_str(), 10, 90, 20, BLACK);
        DrawText(("B: [" + std::to_string(b.x) + ";" + std::to_string(b.y) + "]").c_str(), 10, 110, 20, BLACK);

        DrawCircleV(center, 2.0f, BLACK);
        DrawCircleV(a, 4.0f, RED);
        DrawCircleV(b, 4.0f, BLUE);
        DrawLineV((Vector2){center.x, 0}, (Vector2){center.x, (float)height}, BLACK);
        DrawLineV((Vector2){0, center.y}, (Vector2){(float)width, center.y}, BLACK);

        DrawLineV(center, mouse_pos, BLUE);
        EndDrawing();
    }
}
