#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "hitbox.hpp"
#include "power_up.hpp"

using Poly = shapes::Polygon;
using Circ = shapes::Circle;

TEST_CASE("Polygons", "[hitbox]") {

    Poly poly1({
            (Vector2){ 0, 0 },
            (Vector2){ 1, 0 },
            (Vector2){ 1, 1 },
            (Vector2){ 0, 2 },
            (Vector2){ -1, 1 },
    });

    Poly poly2({
        (Vector2){ 0, 2.1 },
        (Vector2){ 1, 1.1 },
        (Vector2){ 0.5, 3.1 },
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

TEST_CASE("Power Up Test", "[powerup]") {
    /*std::println("GOT RANDOM: {}", powerups::__impl::get_random<powerups::Speed<PowerUpType::Percentage>>());*/
    std::println("GOT RANDOM: {}", powerups::__impl::get_random<powerups::ManaRegen<PowerUpType::Absolute>>());
}
