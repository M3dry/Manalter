#pragma once

#include "raylib.h"

#include <utility>
#include <vector>

#include "assets.hpp"
#include "enemies_spawner.hpp"
#include "item_drops.hpp"
#include "player.hpp"

class Loop {
  public:
    Vector2 screen;
    Player player;
    PlayerStats player_stats;
    Vector2 mouse_pos;
    assets::Store assets;
    bool spellbook_open;
    std::vector<int> registered_keys;

    Enemies enemies;
    ItemDrops item_drops;

    Loop(int width, int height);
    void operator()();

  private:
    double prev_time = 0.0f;
    double accum_time = 0.0f;
    std::vector<std::pair<int, bool>> pressed_keys; // (Key, if it was handled)

    void update();
};
