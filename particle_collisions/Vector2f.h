#pragma once
class Vector2f
{
public:
	float x;
	float y;

	Vector2f() = default;
	Vector2f(float x, float y) noexcept : x(x), y(y) { }

	bool isZero() const noexcept;

	float sum() const noexcept;

	float getSquareLength() const noexcept;			// TODO: Add small documentation in form of comments to each one of these functions. Same for all other functions in the library.
	float getLength() const noexcept;

	Vector2f normalize() const noexcept;

	float operator%(const Vector2f& other) const noexcept;

	// Reflect vector across normal. Vector must be pointing towards surface for correct result.
	Vector2f reflect(const Vector2f& normal) const noexcept;

	Vector2f operator+(const Vector2f& other) const noexcept;

	Vector2f& operator+=(const Vector2f& other) noexcept;			// IMPORTANT: const is super important here so that this can accept r-value refs. const lvalue ref can accept rvalue ref while lvalue ref cannot accept rvalue ref.

	Vector2f operator-(const Vector2f& other) const noexcept;

	Vector2f& operator-=(const Vector2f& other) noexcept;

	Vector2f operator*(float factor) const noexcept;

	friend Vector2f operator*(float factor, const Vector2f& vector) noexcept;	// Implemented to take care of reverse case of the other mult function. Uses friend so that the function is accessible in global scope, which is necessary because float is the main participant now instead of Vector2f when it's this way around.

	Vector2f& operator*=(float factor) noexcept;

	Vector2f operator*(const Vector2f& other) const noexcept;

	Vector2f& operator*=(const Vector2f& other) noexcept;

	Vector2f operator-() const noexcept;
};

