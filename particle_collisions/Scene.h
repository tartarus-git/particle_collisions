#pragma once

#include "Particle.h"
#include "Vector2f.h"
#include <vector>

class Scene
{
public:
	uint32_t width;
	uint32_t height;

	std::vector<Particle> particles;									// TODO: Write a destructor that handles releasing these even though they do it themselves anyway.
	std::vector<size_t> lastIntersectionPartners;
	std::vector<bool> lastIntersectionWasWithWall;
	size_t particleCount;
	size_t lastParticle;

	size_t currentColliderA;						// TODO: These shouldn't be in here since they are just temporarys that don't get used outside of their steps. Making them global would cause issues with multithreading, but making them thread_local would be the best of both worlds. Do that.
	size_t currentColliderB;

	float currentSubStep;						// TODO: This one can also be made thread_local.
	float lowestT;								// TODO: This one too.
	bool noCollisions;							// TODO: Same thing.
	bool boundsCollision;

	void loadSize(unsigned int width, unsigned int height);

	void loadParticles(const std::vector<Particle>& particles, size_t count);
	void loadParticles(const std::vector<Particle>& particles);
	void loadParticles(std::vector<Particle>&& particles, size_t count);
	void loadParticles(std::vector<Particle>&& particles);

	void postLoadInit();

	bool resolveIntersectionWithBounds(size_t particleIndex);
	bool resolveIntersectionWithParticle(size_t aIndex, size_t bIndex);
	void resolveIntersections(size_t particleIndex);

	void sortInvalidatedParticlesAndRemoveMultiples();
	void recalculateInvalidatedData(size_t aIndex, size_t bIndex);

	void findWallCollision(size_t index, const Vector2f& remainingVel);
	void findCollision(size_t aIndex, size_t bIndex, const Vector2f& remainingAlphaVel);
	void reflectCollision();
	void step();
};
