#pragma once

#include <raylib.h>

Vector2 operator +(const Vector2& x, const Vector2& y) {
    return (Vector2){ x.x + y.x, x.y + y.y };
}

Vector2 operator -(const Vector2& x, const Vector2& y) {
    return (Vector2){ x.x - y.x, x.y - y.y };
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

Vector2 operator +(const Vector2& x, const float& c) {
    return (Vector2){ x.x + c, x.y + c };
}

Vector2 operator -(const Vector2& x, const float& c) {
    return (Vector2){ x.x - c, x.y - c };
}

Vector2 operator *(const Vector2& x, const float& c) {
    return (Vector2){ x.x * c, x.y * c };
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
