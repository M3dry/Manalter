#pragma once

#include <cstdint>

template <typename Int, Int points_def, Int percetange_def>
class Stat {
  private:
    Int points = points_def;
    Int percentage = percetange_def;
    Int value = update();

    inline constexpr Int update() {
        return value = points * percentage/100.0f;
    }
  public:
    inline constexpr void add_points(Int p) {
        points += p;
        update();
    }

    inline constexpr void add_percentage(Int per) {
        percentage += per;
        update();
    }

    inline constexpr Int get_points() const {
        return points;
    }

    inline constexpr Int get_percentage() const {
        return percentage;
    }

    inline constexpr Int get() const {
        return value;
    }
};

struct PlayerStats {
    Stat<uint16_t, 10, 100> speed;
    Stat<uint32_t, 100, 100> max_health;
    Stat<uint64_t, 100, 100> max_mana;
    Stat<uint32_t, 1, 100> health_regen;
    Stat<uint32_t, 10, 100> mana_regen;
};
