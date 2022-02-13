#pragma once

#include "Particle.h"
#include "Vector2f.h"
#include <vector>

class Scene
{
public:
	size_t collisionA;
	size_t collisionB;

	std::vector<Particle> particles;
	size_t particleCount;
	size_t lastParticle;
	std::vector<size_t> lastParticleCollisions;
	std::vector<size_t> currentParticleCollisions;
	float stepProgress = 1;
	bool finished = true;

	unsigned int width;
	unsigned int height;

	Scene() = default;

	void setupParticleHelpers();

	Scene(std::vector<Particle> particles, size_t count);
	Scene(std::vector<Particle> particles);
	Scene(std::vector<Particle>&& particles, size_t count);
	Scene(std::vector<Particle>&& particles);
	// TODO: Are there any other cases I should consider?


	void swap(std::vector<Particle> particles) {			// TODO: This method name could be better.
		this->particles.swap(particles);		// TODO: Make sure this is the best way to implement this. Also move to cpp file.
	}

	void findCollision(size_t aIndex, size_t bIndex);
	void findWallCollisions(size_t index);
	void resolveCollisions();
	void step();
};
