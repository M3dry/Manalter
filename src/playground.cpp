#include "quadtree.hpp"

int main(void) {
    auto printer = [](const auto& str, const auto& padding) { std::println("{}{}", padding, *str); };

    quadtree::QuadTree<4, quadtree::pos<const char*>> quadtree(quadtree::Box({0, 0}, {100, 100}));
    quadtree.insert({Vector2{20, 30}, "point1"});
    quadtree.insert({Vector2{20, 70}, "point2"});
    quadtree.insert({Vector2{70, 30}, "point3"});
    quadtree.insert({Vector2{70, 70}, "point4"});
    auto [ix, id] = quadtree.insert({Vector2{30, 40}, "point5"});
    /*quadtree.remove(ix, id);*/
    quadtree.print(printer); // [point1, point5], [point2], [point3], [point4]

    std::println("\nLook up:");
    quadtree.in_box(quadtree::Box({ 30, 30 }, { 60, 60 }), [](const auto& _, auto ix) {
        std::println("\tIX: {}", ix);
    });


    return 0;
}
