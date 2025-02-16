#pragma once

#include "input.hpp"
#include "raylib.h"

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "item_drops.hpp"
#include "player.hpp"

struct Loop;

using Arena = struct Arena {
    Player player;
    Enemies enemies;
    ItemDrops item_drops;

    bool spellbook_open;
    bool paused;

    Arena(Keys& keys);
    ~Arena();

    Arena(Arena&) = delete;
    Arena& operator=(Arena&) = delete;

    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;

    void draw(Loop& loop);
    void update(Loop& loop);
};

using Hub = struct Hub {
    void draw(Loop& loop);
    void update(Loop& loop);

    Hub(Keys& keys);
};

using Loop = struct Loop {
    Vector2 screen;
    Vector2 mouse_pos;
    Keys keys;
    assets::Store assets;

    // nullopt - main menu, player_stats also are nullopt
    // scene has value -> player_stats has value
    std::optional<std::variant<Arena, Hub>> scene;
    std::optional<PlayerStats> player_stats;

    Loop(int width, int height);
    void operator()();

    double prev_time = 0.0f;
    double accum_time = 0.0f;

    void update();
    void load_player();
};
