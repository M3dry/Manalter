#include "hitbox.hpp"

#include <cassert>
#include <ranges>

namespace shapes {
    Polygon::Polygon(std::vector<Vector2> points) : center(std::nullopt), points(points) {
        assert(points.size() > 2);
    }
    Polygon::Polygon(Vector2 center, std::vector<Vector2> points) : center(center), points(points) {
        assert(points.size() > 2);
    }

    void Polygon::draw_lines_2D(Color color) const {
        for (int i = 1; i < points.size(); i++) {
            DrawLineV(points[i - 1], points[i], color);
        }
        DrawLineV(points[points.size() - 1], points[0], color);
    }

    void Polygon::draw_lines_3D(Color color, float y) const {
        for (int i = 1; i < points.size(); i++) {
            DrawLine3D(Vec2ToVec3(points[i - 1], y), Vec2ToVec3(points[i], y), color);
        }
        DrawLine3D(Vec2ToVec3(points[points.size() - 1], y), Vec2ToVec3(points[0], y), color);
    }

    void Polygon::rotate(float angle) {
        if (!center) return;

        float cos_angle = std::cos(angle*DEG2RAD);
        float sin_angle = std::sin(angle*DEG2RAD);

        for (auto& p : points) {
            float x_center = p.x - center->x;
            float y_center = p.y - center->y;
            p.x = x_center*cos_angle - y_center*sin_angle + center->x;
            p.y = x_center*sin_angle + y_center*cos_angle + center->y;
        }
    }

    void Polygon::update(const Vector2& movement) {
        if (center) *center += movement;

        for (auto& p : points) {
            p += movement;
        }
    }

    Circle::Circle(Vector2 center, float radius) : center(center), radius(radius) {}

    void Circle::update(const Vector2& movement) {
        center += movement;
    }
}

bool check_collision(const shapes::Circle& circle1, const shapes::Circle& circle2) {
    float dist_sqr = Vector2DistanceSqr(circle1.center, circle2.center);
    float sum_radius = (circle1.radius + circle2.radius)*(circle1.radius + circle2.radius);
 
    return dist_sqr >= sum_radius;
}

bool check_collision(const shapes::Polygon& poly, const shapes::Circle& circle) {
    // for (int i = 1; i < poly.points.size(); i++) {
    //     Vector2 edge = poly.points[i] - poly.points[i - 1];
    //     Vector2 normal = (Vector2){ -edge.y, edge.x };

    //     float normal_length = Vector2Length(normal);
    //     float min_poly = Vector2DotProduct(poly.points[0], normal)/normal_length;
    //     float max_poly = min_poly;
    //     for (const auto& p : poly.points | std::views::drop(1)) {
    //         float projection = Vector2DotProduct(p, normal)/normal_length;
    //         if (projection < min_poly) min_poly = projection;
    //         else if (projection > max_poly) max_poly = projection;
    //     }

    //     float projected_center = Vector2DotProduct(circle.center, normal)/normal_length;
    //     float min_circle = projected_center + circle.radius;
    //     float max_circle = projected_center - circle.radius;

    //     // true if a separating axis was found => polygons do not collide
    //     if (!(max_poly > min_circle && max_circle > min_poly)) return false;
    // }

    // return true;

    int closest_point = 0;
    float closest_length = Vector2DistanceSqr(poly.points[closest_point], circle.center);
    for (int i = 1; i < poly.points.size(); i++) {
        float length = Vector2DistanceSqr(poly.points[i], circle.center);
        if (length < closest_length) {
            closest_point = i;
            closest_length = length;
        }
    }

    Vector2 edge = poly.points[closest_point] - circle.center;
    Vector2 normal = (Vector2){ -edge.y, edge.x };
    float normal_length = Vector2Length(normal);

    float min_poly = Vector2DotProduct(poly.points[0], normal)/normal_length;
    float max_poly = min_poly;
    for (const auto& p : poly.points | std::views::drop(1)) {
        float projection = Vector2DotProduct(p, normal)/normal_length;
        if (projection < min_poly) min_poly = projection;
        else if (projection > max_poly) max_poly = projection;
    }

    float projected_center = Vector2DotProduct(circle.center, normal)/normal_length;
    float min_circle = projected_center + circle.radius;
    float max_circle = projected_center - circle.radius;

    if (!(max_poly > min_circle && max_circle > min_poly)) return false;
    return true;
}
