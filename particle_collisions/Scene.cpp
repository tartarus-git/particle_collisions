#include "Scene.h"

#include "debugOutput.h"

void Scene::setupParticleHelpers() {
	lastParticleCollisions.resize(particleCount);
	currentParticleCollisions.resize(particleCount);
	for (int i = 0; i < particleCount; i++) {
		lastParticleCollisions[i] = i;					// Last collision for every particle is itself. Signals that no last collision exists yet.
		currentParticleCollisions[i] = i;
	}
}

Scene::Scene(std::vector<Particle> particles, size_t count) : particles(particles), particleCount(count), lastParticle(count - 1) {
	setupParticleHelpers();
}			// TODO: You should probably swap the ones with count with the ones without count because thats the order prioritywise they would be in right?

Scene::Scene(std::vector<Particle> particles) : particles(particles), particleCount(particles.size()), lastParticle(particleCount - 1) {
	setupParticleHelpers();
}

Scene::Scene(std::vector<Particle>&& particles, size_t count) : particles(std::move(particles)), particleCount(count), lastParticle(count - 1) {
	setupParticleHelpers();
}

Scene::Scene(std::vector<Particle>&& particles) : particles(std::move(particles)), particleCount(this->particles.size()), lastParticle(particleCount - 1) {
	setupParticleHelpers();
}

float lowestT;

void Scene::findCollision(size_t aIndex, size_t bIndex) {
	// If last collision was with the same object, it is physically impossible for this collision to be with same object.
	// The main reason this is here is for protection against floating point rounding error:
	// If collision is tested with a particle that is now touching (thanks to collision resolution), due to rounding errors, that object will probably count as a collision.
	// To avoid this, just don't check collisions with that object until this object has collided off of something else and the question can be asked again.
	if (lastParticleCollisions[aIndex] == bIndex) { return; }

	Particle& alphaRef = particles[aIndex];
	Particle& betaRef = particles[bIndex];
	Particle alpha = alphaRef;			// TODO: See if you should do this or not. Potentially compare assembly. Ask on stackoverflow as well maybe.
	alpha.vel *= stepProgress;			// TODO: This is a quick fix, doesn't look good.
	Particle beta = betaRef;
	beta.vel *= stepProgress;

	Vector2f posDiff = alpha.pos - beta.pos;
	float minDist = alpha.radius + beta.radius;
	float squareMinDist = minDist * minDist;
	Vector2f specialVelDiff = alpha.vel - 2 * beta.vel;
	// a and b will equal zero if particles have same velocity and either aren't colliding or are colliding (the latter should theoretically never happen unless the particles get spawned wrong).
	//if (alpha.vel == beta.vel) {			// TODO: This system of doing it is stupid. Also super important: floating point error could creep in between this failing and setting a, which could mean a still becomes 0. Find a way to avoid that.
		float length = posDiff.getLength();
		float dist = length - minDist;
		if (dist < 0) {								// If they ARE somehow inside each other, fix the collision straight away since there is nothing to simulate back to.
			posDiff *= dist / length;
			betaRef.pos += posDiff;
			alphaRef.pos -= posDiff;
			return;
		}
	//}

	// a coefficient
	float a = (alpha.vel - beta.vel) % (alpha.vel - beta.vel);
	if (a == 0) {
		return;
	}

	// b coefficient
	float b = ((alpha.vel - beta.vel) % (alpha.pos - beta.pos)) * 2;

	// c coefficient
	float c = (alpha.pos % alpha.pos) + (beta.pos % beta.pos) - (2 * (alpha.pos % beta.pos)) - (alpha.radius + beta.radius) * (alpha.radius + beta.radius);

	// Construct determinant.
	float r = b * b - 4 * a * c;

	// If no collisions (because parallel trajectories that are too far away from each other for a parallel collision), return.
	// We don't put 1 into earliestCollisions list here because the default value in earliestCollisions is 1. We don't have to do anything.
	if (r < 0) {
		return; }

	// Following gets triggered if 1 collision (parallel lines that are exactly the right distance away), or 2 collisions (all other non-handled collisions).
	// Inefficient to have separate branch for 1 collision because it happens so rarely, just handle it through the math of 2 collision handler.
	r = sqrt(r);
	b = -b;					// Construct full p term.
	float t1 = (b + r) / (2 * a);		// Calculate both possible collisions.
	float t2 = (b - r) / (2 * a);
	float actualT;			// Find the actual collision.
	if (t1 > 1 || t1 < 0) {				// TODO: How does nan compare with normal numbers? Does it always return a false comparison?
		if (t2 > 1 || t2 < 0) {
			return; }							// Both solutions are out of bounds. No collision.
		actualT = t2;												// x2 is the correct solution.
	} else if (t2 > 1 || t2 < 0) { actualT = t1; }					// x1 is the correct solution.
	else { if (t1 < t2) { actualT = t1; } else { actualT = t2; } }	// The lower of the two solutions if the correct solution. This is because the collision that happens the earliest is the real one.
	
	if (actualT <= lowestT) { lowestT = actualT;
	collisionA = aIndex;
	collisionB = bIndex;
	}
	finished = false;				// Make sure everybody knows that this substep still had a collision in it. TODO: See if you can somehow do something smart with stepProgress to signal the same thing without needing to set a bool every time here.
	return;
}

void Scene::findWallCollisions(size_t i) {
	const Particle& a = particles[i];
	if ((a.pos + a.vel).x > width - a.radius || (a.pos + a.vel).x < 0 + a.radius) {
		
	}
}

void Scene::resolveCollisions() {
		Particle& particle = particles[collisionA];
		Particle& other = particles[collisionB];
		Vector2f normal = (other.pos - particle.pos).normalize();
		Vector2f relV = ((particle.vel % normal) * normal) - ((other.vel % normal) * normal);
		particle.vel -= relV;
		other.vel += relV;

			//if (particle.pos.y > height - particle.radius || particle.pos.y < 0 + particle.radius) { particle.vel.y = -particle.vel.y; }
			//if (particle.pos.x > width - particle.radius || particle.pos.x < 0 + particle.radius) { particle.vel.x = -particle.vel.x; }				// This should probably reset last particle collided metric because physics now allows it to hit the same ball again. TODO.
}

void Scene::step() {
	stepProgress = 1;
	while (true) {
		lowestT = 1;
		finished = true;			// Setup for next round. TODO: You should stop setting this in class global and always set it above first for loop.
		// Find the earliest safepoint of all the particles.
		for (int i = 0; i < lastParticle; i++) {
			for (int j = i + 1; j < particleCount; j++) {				// TODO: For loop does first iteration before checking right? If it doesn't that is unnecessary work here.
				findCollision(i, j);
			}
		}
		if (finished) { break; }
		lastParticleCollisions[collisionA] = collisionB;
		lastParticleCollisions[collisionB] = collisionA;
		stepProgress *= lowestT;

		for (int i = 0; i < particleCount; i++) {
			Particle& particle = particles[i];
			particle.pos += particle.vel * stepProgress;
			if (particle.pos.x > width - particle.radius || particle.pos.x < 0 + particle.radius) {
			lastParticleCollisions[i] = i;
				particle.vel.x = -particle.vel.x; }
			if (particle.pos.y > height - particle.radius || particle.pos.y < 0 + particle.radius) {
			lastParticleCollisions[i] = i;
				particle.vel.y = -particle.vel.y; }
		}
		resolveCollisions();
		stepProgress = 1 - stepProgress;
	}
	for (int i = 0; i < particleCount; i++) {
		particles[i].pos += particles[i].vel * stepProgress;
		Particle& particle = particles[i];
			if (particle.pos.x > width - particle.radius || particle.pos.x < 0 + particle.radius) {
			lastParticleCollisions[i] = i;
				particle.vel.x = -particle.vel.x; }
			if (particle.pos.y > height - particle.radius || particle.pos.y < 0 + particle.radius) {
			lastParticleCollisions[i] = i;
				particle.vel.y = -particle.vel.y; }
	}
}
