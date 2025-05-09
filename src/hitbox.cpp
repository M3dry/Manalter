#include "hitbox.hpp"

#include <cassert>
#include <limits>
#include <raylib.h>
#include <utility>

#include "rayhacks.hpp"
#include "raymath.h"

std::pair<float, float> project_polygon(const shapes::Polygon& poly, const Vector2& axis) {
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::min();
    for (const auto& p : poly.points) {
        float projection = Vector2DotProduct(p, axis);
        if (projection < min) min = projection;
        if (projection > max) max = projection;
    }

    return {min, max};
}

std::pair<float, float> project_cirle(const shapes::Circle& circle, const Vector2& axis) {
    Vector2 direction_radius = axis * circle.radius;

    float min = Vector2DotProduct(circle.center + direction_radius, axis);
    float max = Vector2DotProduct(circle.center - direction_radius, axis);

    if (min > max) {
        auto tmp = min;
        min = max;
        max = tmp;
    }

    return {min, max};
}

float project_point(const Vector2& point, const Vector2& axis) {
    auto projection = Vector2DotProduct(point, axis);
    return projection;
}

namespace shapes {
    Polygon::Polygon(std::vector<Vector2> points) : center(std::nullopt), points(points) {
        assert(points.size() > 2);
    }

    Polygon::Polygon(Vector2 center, std::vector<Vector2> points) : center(center), points(points) {
        assert(points.size() > 2);
    }

    Polygon::Polygon(const Rectangle& rec)
        : center({rec.x + rec.width / 2.0f, rec.y + rec.height / 2.0f}),
          points({{rec.x, rec.y},
                  {rec.x + rec.width, rec.y},
                  {rec.x + rec.width, rec.y + rec.height},
                  {rec.x, rec.y + rec.height}}) {
    }

    void Polygon::draw_lines_2D(Color color) const {
        for (std::size_t i = 1; i < points.size(); i++) {
            DrawLineV(points[i - 1], points[i], color);
        }
        DrawLineV(points[points.size() - 1], points[0], color);
    }

    void Polygon::draw_lines_3D(Color color, float y) const {
        for (std::size_t i = 1; i < points.size(); i++) {
            DrawLine3D(Vec2ToVec3(points[i - 1], y), Vec2ToVec3(points[i], y), color);
        }
        DrawLine3D(Vec2ToVec3(points[points.size() - 1], y), Vec2ToVec3(points[0], y), color);
    }

    void Polygon::rotate(float angle) {
        if (!center) return;

        float cos_angle = std::cos(angle * DEG2RAD);
        float sin_angle = std::sin(angle * DEG2RAD);

        for (auto& p : points) {
            float x_center = p.x - center->x;
            float y_center = p.y - center->y;
            p.x = x_center * cos_angle - y_center * sin_angle + center->x;
            p.y = x_center * sin_angle + y_center * cos_angle + center->y;
        }
    }

    void Polygon::translate(const Vector2& movement) {
        if (center) *center += movement;

        for (auto& p : points) {
            p += movement;
        }
    }

    Circle::Circle(Vector2 center, float radius) : center(center), radius(radius) {
    }

    void Circle::translate(const Vector2& movement) {
        center += movement;
    }

    void Circle::draw_3D(Color color, float y, const Vector2& offset) const {
        DrawCircle3D((Vector3){center.x + offset.x, y, center.y + offset.y}, radius,
                     (Vector3){
                         1.0f,
                         0.0f,
                         0.0f,
                     },
                     90.0f, color);
    }
}

bool check_collision(const shapes::Polygon& poly1, const shapes::Polygon& poly2) {
    std::size_t points = poly1.points.size();
    for (std::size_t i = 0; i < points; i++) {
        Vector2 edge = poly1.points[(i + 1) % points] - poly1.points[i];
        Vector2 axis = Vector2Normalize((Vector2){-edge.y, edge.x});

        auto [min_poly1, max_poly1] = project_polygon(poly1, axis);
        auto [min_poly2, max_poly2] = project_polygon(poly2, axis);

        // true if a separating axis was found => polygons do not collide
        if (min_poly1 >= max_poly2 || min_poly2 >= max_poly1) return false;
    }

    points = poly2.points.size();
    for (std::size_t i = 0; i < points; i++) {
        Vector2 edge = poly2.points[(i + 1) % points] - poly2.points[i];
        Vector2 axis = Vector2Normalize((Vector2){-edge.y, edge.x});

        auto [min_poly1, max_poly1] = project_polygon(poly1, axis);
        auto [min_poly2, max_poly2] = project_polygon(poly2, axis);

        // true if a separating axis was found => polygons do not collide
        if (min_poly1 >= max_poly2 || min_poly2 >= max_poly1) return false;
    }

    return true;
}

bool check_collision(const shapes::Circle& circle1, const shapes::Circle& circle2) {
    float dist_sqr = Vector2DistanceSqr(circle1.center, circle2.center);
    float sum_radius = (circle1.radius + circle2.radius) * (circle1.radius + circle2.radius);

    return dist_sqr < sum_radius;
}

bool check_collision(const Vector2& point1, const Vector2& point2) {
    return point1.x == point2.x && point2.y == point2.y;
}

std::size_t find_closest_point(const shapes::Circle& circle, const shapes::Polygon& poly) {
    std::size_t closest_point = 0;

    float min_distance = Vector2DistanceSqr(poly.points[closest_point], circle.center);
    for (std::size_t i = 1; i < poly.points.size(); i++) {
        float distance = Vector2DistanceSqr(poly.points[i], circle.center);
        if (distance < min_distance) {
            min_distance = distance;
            closest_point = i;
        }
    }

    return closest_point;
}

bool check_collision(const shapes::Polygon& poly, const shapes::Circle& circle) {
    {
        std::size_t closest = find_closest_point(circle, poly);
        Vector2 closest_v = poly.points[closest];

        if (closest_v == circle.center) return true;
        Vector2 axis = Vector2Normalize(closest_v - circle.center);

        auto [min_poly, max_poly] = project_polygon(poly, axis);
        auto [min_circle, max_circle] = project_cirle(circle, axis);

        if (min_poly >= max_circle || min_circle >= max_poly) return false;
    }

    std::size_t points = poly.points.size();
    for (std::size_t i = 0; i < points; i++) {
        Vector2 edge = poly.points[(i + 1) % points] - poly.points[i];
        Vector2 axis = Vector2Normalize((Vector2){-edge.y, edge.x});

        auto [min_poly, max_poly] = project_polygon(poly, axis);
        auto [min_circle, max_circle] = project_cirle(circle, axis);

        if (min_poly >= max_circle || min_circle >= max_poly) return false;
    }

    return true;
}

bool check_collision(const shapes::Polygon& poly, const Vector2& point) {
    std::size_t points = poly.points.size();
    for (std::size_t i = 0; i < points; i++) {
        Vector2 edge = poly.points[(i + 1) % points] - poly.points[i];
        Vector2 axis = Vector2Normalize((Vector2){-edge.y, edge.x});

        auto [min_poly, max_poly] = project_polygon(poly, axis);
        auto point_proj = project_point(point, axis);

        // true if a separating axis was found => polygons do not collide
        if (min_poly >= point_proj || point_proj >= max_poly) return false;
    }

    return true;
}

bool check_collision(const shapes::Circle& circle, const Vector2& point) {
    float dist_sqr = Vector2DistanceSqr(circle.center, point);
    /*float sum_radius = (circle.radius + circle2.radius) * (circle1.radius + circle2.radius);*/

    return dist_sqr < circle.radius*circle.radius;
}

void translate(shapes::Polygon& poly, const Vector2& vec) {
    poly.translate(vec);
}

void translate(shapes::Circle& circle, const Vector2& vec) {
    circle.translate(vec);
}

void translate(Vector2& point, const Vector2& vec) {
    point.x += vec.x;
    point.y += vec.y;
}
