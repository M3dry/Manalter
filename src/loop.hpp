#pragma once

#include "enemies.hpp"
#include "input.hpp"
#include "raylib.h"

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "item_drops.hpp"
#include "player.hpp"
#include "ui.hpp"

struct Loop;

struct Arena {
    Player player;
    Enemies enemies;
    ItemDrops item_drops;

    std::optional<hud::SpellBookUI> spellbook_ui;
    bool paused;

    Vector2 mouse_xz;

    Arena(Keys& keys, assets::Store& assets);

    Arena(Arena&) = delete;
    Arena& operator=(Arena&) = delete;

    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;

    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);
};

struct Hub {
    Hub(Keys& keys);

    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);
};

struct Main {
    std::optional<ui::Button> play_button;
    std::optional<ui::Button> exit_button;

    Vector2 last_screen;

    Main(Keys& keys, Vector2 screen);

    void draw(assets::Store& assets, Loop& loop);
    void update(Loop& loop);
};

struct Loop {
    Vector2 screen;
    Keys keys;
    Mouse mouse;
    assets::Store assets;
    EnemyModels enemy_models;

    // nullopt - main menu, player_stats also are nullopt
    // scene has value -> player_stats has value
    std::variant<Arena, Hub, Main> scene;
    std::optional<PlayerStats> player_stats;

    Loop(int width, int height);
    void operator()();

    double prev_time = 0.0f;
    double accum_time = 0.0f;

    void update();
};
