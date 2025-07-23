#include <raylib.h>
#include <ecs.hpp>
#include <raymath.h>

// using TestArch = ecs::Archetype<ecs::Entity, int, float>;
// using ECS = ecs::build<TestArch>;
//
// int main() {
//     auto ecs = ECS();
//
//     std::size_t counter = 1;
//     ecs.static_emplace_entity<TestArch>(ecs::null_id, 0, 0.0f);
//     ecs.make_system<TestArch>().run<true>([&](ecs::Entity current, ecs::Entity& _, int& i, float& f) {
//         if (counter++ == 1000) return;
//
//         ecs.static_emplace_entity<TestArch>(current, i + 1, (float)(i + 1)/10.0f);
//     });
//     // for (std::size_t i = 0; i < 1000; i++) {
//     //     last = ecs.static_emplace_entity<TestArch>(last, i, (float)i/10.0f);
//     // }
//
//     counter = 0;
//     ecs.make_system<TestArch>().run<true>([&](ecs::Entity current, ecs::Entity& last, int& i, float& f) {
//         assert(current == (std::size_t)i);
//         assert(last == (std::size_t)(i - 1));
//         assert(f == (float)i/10.0f);
//         counter++;
//     });
//
//     assert(counter == 1000);
// }

using position = Vector2;
using parent = ecs::Entity;
using radius = float;
using tail = bool;
using LinkArchetype = ecs::Archetype<parent, position, radius, tail>;

using ParticleECS = ecs::build<LinkArchetype>;

int main() {
    InitWindow(0, 0, "MOTE demo");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    Vector2 screen = Vector2{(float)GetScreenWidth(), (float)GetScreenHeight()};

    auto ecs = ParticleECS();
    auto s = ecs.make_system<LinkArchetype>();

    ecs.static_emplace_entity<LinkArchetype>(ecs::null_id, Vector2{screen.x/2.0f, screen.y/2.0f}, 50.0f, true);

    float prev_time = 0.;
    bool print = true;
    while (!WindowShouldClose()) {
        float current_time = static_cast<float>(GetTime());
        float delta_time = current_time - prev_time;
        prev_time = current_time;

        s.run<true>([&](auto& current, parent& parent, position& pos, radius& rad, tail& tail) {
            if (parent == ecs::null_id) {
                if (tail) {
                    tail = false;
                    ecs.static_emplace_entity<LinkArchetype>(current, pos, rad/1.05f, true);
                }

                return;
            }
            if (!tail) return;

            position& parent_pos = std::get<0>(*ecs.static_get<LinkArchetype, position>(parent));
            if (pos.y - parent_pos.y <= 1.05f*rad) {
                pos = Vector2Add(pos, Vector2Scale(Vector2{0.0f, 100.0f}, delta_time));
            } else if (rad/1.05f >= 30.0f) {
                tail = false;
                ecs.static_emplace_entity<LinkArchetype>(current, pos, rad/1.05f, true);
            }
        });

        PollInputEvents();
        BeginDrawing();
            ClearBackground(DARKGRAY);

            s.run([&](parent& parent, position& pos, radius& rad, tail& _) {
                if (parent != ecs::null_id) {
                    auto exists = ecs.static_get<LinkArchetype, position>(parent);
                    assert(exists);

                    auto parent_pos = std::get<0>(*exists);
                    DrawLineV(parent_pos, pos, WHITE);
                }
                DrawCircleLinesV(pos, rad, RED);
            });
        EndDrawing();
        SwapScreenBuffer();
    }

    CloseWindow();
    return 0;
}
