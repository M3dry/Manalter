#include "quadtree.hpp"

#include "rayhacks.hpp"
#include <raylib.h>

int main(void) {
    InitWindow(0, 0, "skibidi");

    Vector2 screen = { (float)GetScreenWidth(), (float)GetScreenHeight() };

    auto printer = [](const auto& str, const auto& padding) { std::println("{}{}", padding, *str); };

    quadtree::QuadTree<4, quadtree::pos<float>> quadtree(quadtree::Box({0, 0}, {screen.x, screen.y}));
    quadtree.insert({Vector2{20, 30}, 5.0f});
    quadtree.insert({Vector2{20, 27}, 5.0f});
    quadtree.insert({Vector2{70, 67}, 5.0f});
    quadtree.insert({Vector2{70, 70}, 5.0f});
    auto [ix, id] = quadtree.insert({Vector2{30, 40}, 5.0f});


    bool resolved = false;
    while (!WindowShouldClose()) {
        PollInputEvents();
        BeginDrawing();
        ClearBackground(WHITE);

        for (const auto& radius : quadtree.data) {
            DrawCircleV(radius.val.position(), *radius.val, RED);
        }

        if (resolved) DrawText("RESOLVED", screen.x - 200, screen.y - 30, 20, BLACK);
        EndDrawing();
        SwapScreenBuffer();

        if (IsKeyPressed(KEY_SPACE)) {
            resolved = true;
/*            quadtree.resolve_collisions(*/
/*                [](const auto& p1, const auto& p2) -> std::optional<Vector2> {*/
/*                    auto dist_sqr = Vector2DistanceSqr(p1.position(), p2.position());*/
/**/
/*#define SQR(x) ((x) * (x))*/
/*                    if (dist_sqr <= SQR(*p1 + *p2)) {*/
/*                        float distance = std::sqrt(dist_sqr);*/
/*                        if (distance == 0) {*/
/*                            // If centers coincide, pick a default direction (or a random one)*/
/*                            return Vector2Scale({1, 0}, *p1);*/
/*                        }*/
/*                        // Normalize the difference vector:*/
/*                        auto collision_normal = Vector2Scale((p1.position() - p2.position()), 1.0f / distance);*/
/*                        // Compute the penetration vector:*/
/*                        return collision_normal * ((*p1 + *p2) - distance);*/
/*                    }*/
/*#undef SQR*/
/**/
/*                    return std::nullopt;*/
/*                },*/
/*                5);*/
        }
    }

    CloseWindow();

    return 0;
}
