#include "quadtree.hpp"

#include "rayhacks.hpp"

int main(void) {
    auto printer = [](const auto& str, const auto& padding) { std::println("{}{}", padding, *str); };

    quadtree::QuadTree<4, quadtree::pos<float>> quadtree(quadtree::Box({0, 0}, {100, 100}));
    quadtree.insert({Vector2{20, 30}, 5.0f});
    quadtree.insert({Vector2{20, 27}, 5.0f});
    quadtree.insert({Vector2{70, 67}, 5.0f});
    quadtree.insert({Vector2{70, 70}, 5.0f});
    auto [ix, id] = quadtree.insert({Vector2{30, 40}, 5.0f});
    /*quadtree.remove(ix, id);*/

    quadtree.rebuild();
    quadtree.resolve_collisions([](const auto& e1, const auto& e2) -> std::optional<Vector2> {
        auto dist_sqr = Vector2DistanceSqr(e1.position(), e2.position());

#define SQR(x) ((x)*(x))
        if (dist_sqr <= SQR(*e1+ *e2)) {
            if (dist_sqr == 0) {
                return Vector2Scale({ 1, 0 }, *e1);
            }

            auto collision_normal = Vector2Scale((e1.position() - e2.position()), std::sqrt(dist_sqr));
            return collision_normal * ((*e1 + *e2) - std::sqrt(dist_sqr));
        }
#undef SQR

        return std::nullopt;
    }, 5);
    quadtree.print(printer); // [point1, point5], [point2], [point3], [point4]

    return 0;
}
