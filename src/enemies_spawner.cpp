#include "enemies_spawner.hpp"

#include "rayhacks.hpp"

bool _Enemies::spawn(const Vector2& player_pos) {

}

void _Enemies::update_target(const Vector2& player_pos) {
    if (player_pos == last_player_pos) {
        return;
    }

    for (auto& enemy : enemies) {
        enemy.update_target(player_pos);
    }

    last_player_pos = player_pos;
}

void _Enemies::deal_damage(std::size_t enemy, uint32_t damage, Element element) {
        // TODO: Elemental damage scaling
        enemies[enemy].health -= damage;

        if (enemies[enemy].health <= 0) {
            // NOTE: should probably do something more efficient here
            enemies.erase(enemies.begin() + enemy);
        }
}
