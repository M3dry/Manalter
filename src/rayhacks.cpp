#include "rayhacks.hpp"

Vector3 Vec2ToVec3(const Vector2& vec, float y) {
    return (Vector3){vec.x, y, vec.y};
}

#ifndef PLATFORM_WEB
Vector2 operator+(const Vector2& x, const Vector2& y) {
    return (Vector2){x.x + y.x, x.y + y.y};
}

Vector2 operator-(const Vector2& x, const Vector2& y) {
    return (Vector2){x.x - y.x, x.y - y.y};
}

Vector2 operator-(const Vector2& x) {
    return (Vector2){ -x.x, -x.y };
}

Vector2& operator+=(Vector2& lhs, const Vector2& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

Vector2& operator-=(Vector2& lhs, const Vector2& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

Vector2 operator+(const Vector2& x, const float& c) {
    return (Vector2){x.x + c, x.y + c};
}

Vector2 operator-(const Vector2& x, const float& c) {
    return (Vector2){x.x - c, x.y - c};
}

Vector2 operator*(const Vector2& x, const float& c) {
    return (Vector2){x.x * c, x.y * c};
}

Vector2& operator+=(Vector2& lhs, const float& c) {
    lhs.x += c;
    lhs.y += c;
    return lhs;
}

Vector2& operator-=(Vector2& lhs, const float& c) {
    lhs.x -= c;
    lhs.y -= c;
    return lhs;
}

Vector2& operator*=(Vector2& lhs, const float& c) {
    lhs.x *= c;
    lhs.y *= c;
    return lhs;
}

Vector2& operator/=(Vector2& lhs, const float& c) {
    lhs.x /= c;
    lhs.y /= c;
    return lhs;
}

bool operator==(const Vector2& lhs, const Vector2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}
#endif
