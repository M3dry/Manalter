#pragma once

#include "particle_system.hpp"
#include <limits>
#include <raylib.h>

namespace effect {
    struct Plosion {
        enum Type {
            Ex,
            Im,
        };

        Type type;
        float radius;
        std::size_t particle_count = 5000;
        std::size_t max_emit = particle_count;
        float emit_rate = static_cast<float>(std::numeric_limits<std::size_t>::max());
        float particle_size_scale = 1.0f;
        std::optional<float> floor_y = std::nullopt;
        float center_y = 0.0f;
        std::pair<float, float> lifetime = {2.5f, 3.5f};
        std::pair<float, float> velocity_scale = {3.0f, 30.0f};
        Vector3 acceleration = {20.0, 20.0f, 20.0f };
        std::pair<std::pair<Color, float>, std::pair<Color, float>> color;

        particle_system::System operator()(Vector2 origin);
    };

    struct FlyingBall {
        Vector3 direction;
        float radius;
        std::size_t particle_count = 5000;
        float particle_size_scale = 1.0f;
        std::optional<float> floor_y = 0.0f;
        std::pair<float, float> lifetime = {2.5f, 3.5f};
        Vector3 acceleration = { 20.0f, 20.0f, 20.0f};

        particle_system::System operator()(Vector2 origin);
    };

    struct ItemDrop {
        float y = 0.0f;
        std::size_t particle_count = 200;
        float emit_rate = 50000;

        particle_system::System operator()(Vector2 origin, const Color& rarity_color);
    };
}

using Effect = std::variant<effect::Plosion>;

namespace effects {
    // .first = id
    // .second = ix
    using Id = std::pair<std::size_t, std::size_t>;

    Id push_effect(particle_system::System&& eff);
    void pop_effect(Id id);

    void update(float dt);
    void draw();
    void clean();
}
