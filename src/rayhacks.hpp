#pragma once

#include <raylib.h>

Vector3 Vec2ToVec3(const Vector2& vec, float y);
#ifndef PLATFORM_WEB
Vector2 operator+(const Vector2& x, const Vector2& y);
Vector2 operator-(const Vector2& x, const Vector2& y);
Vector2& operator+=(Vector2& lhs, const Vector2& rhs);
Vector2& operator-=(Vector2& lhs, const Vector2& rhs);
Vector2 operator+(const Vector2& x, const float& c);
Vector2 operator-(const Vector2& x, const float& c);
Vector2 operator*(const Vector2& x, const float& c);
Vector2& operator+=(Vector2& lhs, const float& c);
Vector2& operator-=(Vector2& lhs, const float& c);
Vector2& operator*=(Vector2& lhs, const float& c);
Vector2& operator/=(Vector2& lhs, const float& c);
bool operator==(const Vector2& lhs, const Vector2& rhs);
#endif
