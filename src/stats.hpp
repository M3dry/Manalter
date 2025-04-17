#pragma once

#include "seria_deser.hpp"
#include <cstdint>
#include <ostream>

template <SeriaDeser Int, Int points_def, Int percetange_def> class Stat {
  private:
    Int points = points_def;
    Int percentage = percetange_def;
    Int value = update();

    inline constexpr Int update() {
        return value = static_cast<Int>(static_cast<float>(points) * static_cast<float>(percentage) / 100.0f);
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

    void serialize(std::ostream& out) const {
        seria_deser::serialize(points, out);
        seria_deser::serialize(percentage, out);
    }

    static Stat deserialize(std::istream& in, version version) {
        Stat stat;

        stat.points = seria_deser::deserialize<Int>(in, version),
        stat.percentage = seria_deser::deserialize<Int>(in, version),
        stat.update();

        return stat;
    }
};

struct PlayerStats {
    Stat<uint16_t, 10, 100> speed;
    Stat<uint32_t, 100, 100> max_health;
    Stat<uint64_t, 100, 100> max_mana;
    Stat<uint32_t, 1, 100> health_regen;
    Stat<uint32_t, 10, 100> mana_regen;
};
