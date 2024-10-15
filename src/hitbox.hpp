#pragma once

#include <vector>
#include <concepts>
#include <cassert>
#include <limits>
#include <optional>
#include <ranges>

#include <raylib.h>
#include <raymath.h>

#include "rayhacks.hpp"

namespace shapes {
    using Polygon = struct Polygon {
        std::optional<Vector2> center;
        std::vector<Vector2> points;

        Polygon(std::vector<Vector2> points) : center(std::nullopt), points(points) {
            assert(points.size() > 2);
        }
        Polygon(Vector2 center, std::vector<Vector2> points) : center(center), points(points) {
            assert(points.size() > 2);
        }

        void draw_lines(Color color) const {
            for (int i = 1; i < points.size(); i++) {
                DrawLineV(points[i - 1], points[i], color);
            }
            DrawLineV(points[points.size() - 1], points[0], color);
        }

        // angle in degrees, counterclockwise
        // does nothing if center is nullopt
        void rotate(float angle) {
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

        void update(const Vector2& movement) {
            if (center) *center += movement;

            for (auto& p : points) {
                p += movement;
            }
        }
    };

    using Circle = struct Circle {
        Vector2 center;
        float radius;

        Circle(float radius, Vector2 center) {}

        void update(const Vector2& movement) {
            center += movement;
        }
    };
}

namespace SAT {
    bool check_collision(const shapes::Polygon& poly1, const shapes::Polygon& poly2) {
        for (int i = 1; i < poly1.points.size(); i++) {
            Vector2 edge = poly1.points[i] - poly1.points[i - 1];
            Vector2 normal = (Vector2){ -edge.y, edge.x };

            float normal_length = Vector2Length(normal);
            float min_poly1 = Vector2DotProduct(poly1.points[0], normal)/normal_length;
            float max_poly1 = min_poly1;
            for (const auto& p : poly1.points | std::views::drop(1)) {
                float projection = Vector2DotProduct(p, normal)/normal_length;
                if (projection < min_poly1) min_poly1 = projection;
                else if (projection > max_poly1) max_poly1 = projection;
            }

            float min_poly2 = Vector2DotProduct(poly2.points[0], normal)/normal_length;
            float max_poly2 = min_poly2;
            for (const auto& p : poly2.points | std::views::drop(1)) {
                float projection = Vector2DotProduct(p, normal)/normal_length;
                if (projection < min_poly2) min_poly2 = projection;
                else if (projection > max_poly2) max_poly2 = projection;
            }

            // true if a separating axis was found => polygons do not collide
            if (!(max_poly1 > min_poly2 && max_poly2 > min_poly1)) return false;
        }

        for (int i = 1; i < poly2.points.size(); i++) {
            Vector2 edge = poly2.points[i] - poly2.points[i - 1];
            Vector2 normal = (Vector2){ -edge.y, edge.x };

            float normal_length = Vector2Length(normal);
            float min_poly1 = Vector2DotProduct(poly1.points[0], normal)/normal_length;
            float max_poly1 = min_poly1;
            for (const auto& p : poly1.points | std::views::drop(1)) {
                float projection = Vector2DotProduct(p, normal)/normal_length;
                if (projection < min_poly1) min_poly1 = projection;
                else if (projection > max_poly1) max_poly1 = projection;
            }

            float min_poly2 = Vector2DotProduct(poly2.points[0], normal)/normal_length;
            float max_poly2 = min_poly2;
            for (const auto& p : poly2.points | std::views::drop(1)) {
                float projection = Vector2DotProduct(p, normal)/normal_length;
                if (projection < min_poly2) min_poly2 = projection;
                else if (projection > max_poly2) max_poly2 = projection;
            }

            // true if a separating axis was found => polygons do not collide
            if (!(max_poly1 > min_poly2 && max_poly2 > min_poly1)) return false;
        }

        return true;
    }

    bool check_collision(const shapes::Polygon& poly, const shapes::Circle& circle) {
        return false;
    }
}
