#pragma once

// vector consisting of 2 floats
class Vector2f
{
public:
	float x;
	float y;

	Vector2f() = default;
	Vector2f(float x, float y) noexcept : x(x), y(y) { }

	bool isZero() const noexcept;							// returns true if the both x and y equal 0 (can get hairy because of floating point)

	float sum() const noexcept;							// returns the sum of x and y

	// Returns length of vector squared. Reason to use this instead of squaring getLength is: getLength contains a sqrt, which this simply avoids, which is more efficient than
	// squaring a sqrt.
	float getSquareLength() const noexcept;
	float getLength() const noexcept;						// returns length of vector

	Vector2f normalize() const noexcept;						// returns normalized vector

	float operator%(const Vector2f& other) const noexcept;				// returns dot product of vector with another vector

	// Reflect vector across normal. Vector must be pointing towards surface for correct result.
	Vector2f reflect(const Vector2f& normal) const noexcept;

	Vector2f operator+(const Vector2f& other) const noexcept;			// returns sum of vector with another vector

	Vector2f& operator+=(const Vector2f& other) noexcept;				// add vector to this vector as well as return the new version of this vector
	// SIDE-NOTE FOR ME: const is super important here so that this can accept r-value refs. const lvalue ref can accept rvalue ref while lvalue ref cannot accept rvalue ref.

	Vector2f operator-(const Vector2f& other) const noexcept;			// returns difference between another vector and this vector

	Vector2f& operator-=(const Vector2f& other) noexcept;				// subtract vector from this vector as well as return new version of this vector

	Vector2f operator*(float factor) const noexcept;				// returns the product of this vector and a scalar value

	friend Vector2f operator*(float factor, const Vector2f& vector) noexcept;	// returns the product of a scalar value and a vector
	// SIDE-NOTE FOR ME: Implemented to take care of reverse case of the other mult function. Uses friend so that the function is accessible in global scope, which is necessary because float is the main participant now instead of Vector2f when it's this way around. BTW: This is not the main use-case for friend. The reason it exists is a bit different.

	Vector2f& operator*=(float factor) noexcept;					// multiply this vector with a scalar value as well as return new version of this vector

	Vector2f operator*(const Vector2f& other) const noexcept;			// returns product of this vector and another vector (element by element)

	Vector2f& operator*=(const Vector2f& other) noexcept;				// multiply vector with another vector (element by element) as well as return new version

	Vector2f operator-() const noexcept;						// return the opposite vector (a vector pointing in the opposite direction)

	bool operator==(const Vector2f& other) const noexcept;				// returns true if two vectors are the same (can get hairy because of floating-point)
};

