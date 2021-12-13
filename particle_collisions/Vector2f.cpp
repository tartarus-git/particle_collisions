#include "Vector2f.h"

#include <cmath>

bool Vector2f::isZero() const noexcept { return !(x || y); }

float Vector2f::sum() const noexcept { return x + y; }

float Vector2f::getSquareLength() const noexcept { return x * x + y * y; }
float Vector2f::getLength() const noexcept { return sqrt(getSquareLength()); }

Vector2f Vector2f::normalize() const noexcept { float length = getLength(); return Vector2f(x / length, y / length); }

float Vector2f::operator%(const Vector2f& other) const noexcept { return x * other.x + y * other.y; }

Vector2f Vector2f::reflect(const Vector2f& normal) const noexcept { return *this - normal * (*this % normal) * 2; }

Vector2f Vector2f::operator+(const Vector2f& other) const noexcept { return Vector2f(x + other.x, y + other.y); }

Vector2f& Vector2f::operator+=(const Vector2f& other) noexcept { x += other.x; y += other.y; return *this; }

Vector2f Vector2f::operator-(const Vector2f& other) const noexcept { return Vector2f(x - other.x, y - other.y); }

Vector2f& Vector2f::operator-=(const Vector2f& other) noexcept { x -= other.x; y -= other.y; return *this; }

Vector2f Vector2f::operator*(float factor) const noexcept { return Vector2f(x * factor, y * factor); }

Vector2f operator*(float factor, Vector2f vector) noexcept { return vector * factor; }

Vector2f& Vector2f::operator*=(float factor) noexcept { x *= factor; y *= factor; return *this; }

Vector2f Vector2f::operator*(const Vector2f& other) const noexcept { return Vector2f(x * other.x, y * other.y); }

Vector2f& Vector2f::operator*=(const Vector2f& other) noexcept { x *= other.x; y *= other.y; return *this; }

Vector2f Vector2f::operator-() const noexcept { return Vector2f(-x, -y); }

bool Vector2f::operator==(const Vector2f& other) const noexcept { return x == other.x && y == other.y; }
