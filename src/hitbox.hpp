#pragma once

#include <optional>
#include <vector>

#include <raylib.h>
#include <raymath.h>

namespace shapes {
    using Polygon = struct Polygon {
        std::optional<Vector2> center;
        std::vector<Vector2> points;

        Polygon(std::vector<Vector2> points);
        Polygon(Vector2 center, std::vector<Vector2> points);

        void draw_lines_2D(Color color) const;
        void draw_lines_3D(Color color, float y) const;
        // angle in degrees, counterclockwise
        // does nothing if center is nullopt
        void rotate(float angle);
        void update(const Vector2& movement);
    };

    using Circle = struct Circle {
        Vector2 center;
        float radius;

        Circle(Vector2 center, float radius);

        void update(const Vector2& movement);

        void draw_3D(Color color, float y) const;
    };
}

// TODO: better concept for this mess
bool check_collision(const shapes::Polygon& poly1, const shapes::Polygon& poly2);
bool check_collision(const shapes::Circle& circle1, const shapes::Circle& circle2);
bool check_collision(const Vector2& point1, const Vector2& point2);
bool check_collision(const shapes::Polygon& poly, const shapes::Circle& circle);
bool check_collision(const shapes::Polygon& poly, const Vector2& point2);
bool check_collision(const shapes::Circle& circle, const Vector2& point);

template <typename T>
concept Shape = requires (const T& shape, const shapes::Polygon& poly) {
    { check_collision(shape, shape) } -> std::same_as<bool>;
    { check_collision(poly, shape) } -> std::same_as<bool>;
};

static_assert(Shape<shapes::Circle>);
static_assert(Shape<shapes::Polygon>);
static_assert(Shape<Vector2>);
