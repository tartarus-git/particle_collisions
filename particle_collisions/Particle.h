#pragma once

#include "Vector2f.h"

class Particle
{
public:
	Vector2f pos;
	Vector2f vel;
	float radius;
	float mass;

	bool lastInteractionWasIntersection = false;

	Particle() = default;
	Particle(const Vector2f& pos, const Vector2f& vel, float radius, float mass) noexcept : pos(pos), vel(vel), radius(radius), mass(mass) { }

	void update() noexcept;
};

