#include <catch2/catch_test_macros.hpp>

#include "raylib.h"
#include "enemies_spawner.hpp"
#include "hitbox.hpp"
#include "utility.hpp"

// NOTE: really don't like how I have to create a window just to test spawning
// I don't think there's anything that can be done about this, as the Enemy class requires the model information to work properly
// TODO: create dummy models with so that I don't have to cd back to project level
TEST_CASE("Spawn every second", "[enemy][raylib]") {
    InitWindow(100, 100, "TEST");

    auto n = 32;
    Enemies enemies(100);

    EnemyModels enemy_models{};

    for (int i = 0; i < TICKS*n; i++) {
        enemies.tick(shapes::Circle(Vector2Zero(), 1.0f), enemy_models);
    }

    REQUIRE(enemies.enemies.data.size() == n);

    CloseWindow();
}
