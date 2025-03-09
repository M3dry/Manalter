#pragma once

#include <algorithm>
#include <limits>
#include <raylib.h>

namespace quadtree {
    static constinit float inf = std::numeric_limits<float>::infinity();

    struct Box {
        Vector2 min{inf, inf };
        Vector2 max{-inf, -inf };

        void extend(const Vector2& p) {
            min.x = std::min(min.x, p.x);
            min.y = std::min(min.y, p.y);
            max.x = std::max(max.x, p.x);
            max.y = std::max(max.y, p.y);
        }
    };

    Vector2 middle(const Vector2& p1, const Vector2& p2);
}
