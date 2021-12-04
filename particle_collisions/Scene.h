#pragma once

#include "Particle.h"
#include <vector>

class Scene
{
public:
	std::vector<Particle> particles;
	size_t particleCount;
	size_t lastParticle;		// TODO: Make the constructors initialize this one.
	std::vector<size_t> lastParticleCollisions;		// TODO: Make the constructors make this.
	std::vector<float> earliestCollisions;

	Scene() = default;

	void setupHelperVectors();

	Scene(std::vector<Particle> particles, size_t count);
	Scene(std::vector<Particle> particles);
	Scene(std::vector<Particle>&& particles, size_t count);
	Scene(std::vector<Particle>&& particles);
	// TODO: Are there any other cases I should consider?


	void swap(std::vector<Particle> particles) {			// TODO: This method name could be better.
		this->particles.swap(particles);		// TODO: Make sure this is the best way to implement this. Also move to cpp file.
	}

	void findCollision(size_t aIndex, size_t bIndex);
	void resolveCollisions();
	void step();
};
