#include "quadtree.hpp"

namespace quadtree {
    Vector2 middle(const Vector2& p1, const Vector2& p2) {
        return (Vector2){ (p1.x + p2.x)/2.0f, (p1.y + p2.y)/2.0f };
    }
}
