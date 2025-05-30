#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "hitbox.hpp"

using Poly = shapes::Polygon;
using Circ = shapes::Circle;

TEST_CASE("Polygons", "[hitbox]") {

    Poly poly1({
            (Vector2){ 0.0f, 0.0f },
            (Vector2){ 1.0f, 0.0f },
            (Vector2){ 1.0f, 1.0f },
            (Vector2){ 0.0f, 2.0f },
            (Vector2){ -1.0f, 1.0f },
    });

    Poly poly2({
        (Vector2){ 0.f, 2.1f },
        (Vector2){ 1.f, 1.1f },
        (Vector2){ 0.5f, 3.1f },
    });


    // TODO: get center from points automatically
    /*auto rotations = GENERATE(0, 30, 45, 60, 90);*/
    /*poly2.rotate(rotations);*/

    REQUIRE(check_collision(poly1, poly2) == check_collision(poly2, poly1));

    Vector2 point = { 0.5, 1 };

    REQUIRE(check_collision(poly1, point));
    REQUIRE(!check_collision(poly2, point));

    Circ circle({ 0.5, 0.5 }, 10);

    REQUIRE(check_collision(poly1, circle));
    REQUIRE(check_collision(poly2, circle));
}
