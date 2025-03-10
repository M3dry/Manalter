#include "quadtree.hpp"

int main(void) {
    auto printer = [](const auto& str, const auto& padding) {
        std::println("{}{}", padding, str);
    };

    quadtree::QuadTree<4, const char*> quadtree(quadtree::Box{
        .min = {0, 0},
        .max = {100, 100},
    });
    quadtree.insert(Vector2{ 20, 30 }, "point1");
    quadtree.insert(Vector2{ 20, 70 }, "point2");
    quadtree.insert(Vector2{ 70, 30 }, "point3");
    quadtree.insert(Vector2{ 70, 70 }, "point4");
    quadtree.insert(Vector2{ 30, 40 }, "point5");
    quadtree.print(printer); // [point1, point5], [point2], [point3], [point4]

    return 0;
}
