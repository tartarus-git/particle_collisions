#include "Scene.h"

#include "debugOutput.h"

void Scene::setupParticleHelpers() {
	lastParticleCollisions.resize(particleCount);
	for (int i = 0; i < particleCount; i++) {
		lastParticleCollisions[i] = i;					// Last collision for every particle is itself. Signals that no last collision exists yet.
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

void Scene::findCollision(size_t aIndex, size_t bIndex) {
	// If last collision was with the same object, it is physically impossible for this collision to be with same object.
	// The main reason this is here is for protection against floating point rounding error:
	// If collision is tested with a particle that is now touching (thanks to collision resolution), due to rounding errors, that object will probably count as a collision.
	// To avoid this, just don't check collisions with that object until this object has collided off of something else and the question can be asked again.
	if (lastParticleCollisions[aIndex] == bIndex) { return; }			// TODO: How does this issue get resolved. They've already collided, but if we leave this like this, the collider will collide them again, thats bad. Fix this whole system, the structure is terrible.

	Particle& alphaRef = particles[aIndex];
	Particle& betaRef = particles[bIndex];
	Particle alpha = alphaRef;			// TODO: See if you should do this or not. Potentially compare assembly. Ask on stackoverflow as well maybe.
	alpha.vel *= 1 - prevStepProgress;			// TODO: This is a quick fix, doesn't look good.
	Particle beta = betaRef;
	beta.vel *= 1 - prevStepProgress;

	/*Vector2f posDiff = alpha.pos - beta.pos;
	float minDist = alpha.radius + beta.radius;
	float squareMinDist = minDist * minDist;
	Vector2f specialVelDiff = alpha.vel - 2 * beta.vel;
	// a and b will equal zero if particles have same velocity and either aren't colliding or are colliding (the latter should theoretically never happen unless the particles get spawned wrong).
	if (specialVelDiff == -beta.vel) {			// TODO: This system of doing it is stupid. Also super important: floating point error could creep in between this failing and setting a, which could mean a still becomes 0. Find a way to avoid that.
		float length = posDiff.getLength();
		float dist = length - squareMinDist;
		if (dist < 0) {								// If they ARE somehow inside each other, fix the collision straight away since there is nothing to simulate back to.
			posDiff *= dist / length;
			betaRef.pos += posDiff;
			alphaRef.pos -= posDiff;
		}
		return;
	}*/

	// a coefficient
	float a = (alpha.vel - beta.vel) % (alpha.vel - beta.vel);

	// b coefficient
	float b = (alpha.vel + beta.vel) % (alpha.pos - beta.pos) * 2;

	// c coefficient
	float c = (alpha.pos % alpha.pos) + (beta.pos % beta.pos) - (alpha.radius + beta.radius) * (alpha.radius + beta.radius);

	// Construct determinant.
	float r = b * b - 4 * a * c;

	// If no collisions (because parallel trajectories that are too far away from each other for a parallel collision), return.
	// We don't put 1 into earliestCollisions list here because the default value in earliestCollisions is 1. We don't have to do anything.
	if (r < 0) {
			lastParticleCollisions[aIndex] = aIndex;
			lastParticleCollisions[bIndex] = bIndex;
		return; }

	// Following gets triggered if 1 collision (parallel lines that are exactly the right distance away), or 2 collisions (all other non-handled collisions).
	// Inefficient to have separate branch for 1 collision because it happens so rarely, just handle it through the math of 2 collision handler.
	r = sqrt(r);
	b = -b;					// Construct full p term.
	float t1 = b + r;		// Calculate both possible collisions.
	float t2 = b - r;
	float actualT;			// Find the actual collision.
	if (t1 > 1 || t1 <= 0) {				// TODO: How does nan compare with normal numbers? Does it always return a false comparison?
		if (t2 > 1 || t2 <= 0) {
			lastParticleCollisions[aIndex] = aIndex;
			lastParticleCollisions[bIndex] = bIndex;
			return; }							// Both solutions are out of bounds. No collision.
		actualT = t2;												// x2 is the correct solution.
	} else if (t2 > 1 || t2 <= 0) { actualT = t1; }					// x1 is the correct solution.
	else { if (t1 < t2) { actualT = t1; } else { actualT = t2; } }	// The lower of the two solutions if the correct solution. This is because the collision that happens the earliest is the real one.
	
	if (actualT < stepProgress) { stepProgress = actualT; }
	finished = false;				// Make sure everybody knows that this substep still had a collision in it. TODO: See if you can somehow do something smart with stepProgress to signal the same thing without needing to set a bool every time here.
	lastParticleCollisions[aIndex] = bIndex;
	lastParticleCollisions[bIndex] = bIndex;
	return;
}

void Scene::resolveCollisions() {
	for (int i = 0; i < particleCount; i++) {
		if (lastParticleCollisions[i] == i) { continue; }			// If nothing to collide with for this particle or if this particle's collision is recorded in another particles lastParticleCollision, do nothing.
		Particle& particle = particles[i];
		particle.pos += particle.vel * stepProgress;
		Particle& other = particles[lastParticleCollisions[i]];				// TODO: You gotta move the other particle in this function as well or else this ain't gonna work.
		other.pos += other.vel * stepProgress;
		Vector2f normal = (other.pos - particle.pos).normalize();
		Vector2f relV = ((particle.vel % normal) * normal) - ((other.vel % normal) * normal);
		particle.vel -= relV;
		other.vel += relV;
	}
}

void Scene::step() {
	while (true) {
		prevStepProgress = stepProgress;
		stepProgress = 1;
		// Find the earliest safepoint of all the particles.
		for (int i = 0; i < lastParticle; i++) {
			for (int j = i + 1; j < particleCount; j++) {				// TODO: For loop does first iteration before checking right? If it doesn't that is unnecessary work here.
				findCollision(i, j);
			}
		}
		stepProgress = prevStepProgress + (1 - prevStepProgress) * stepProgress;
		if (finished) { break; }
		finished = true;			// Setup for next round. TODO: You should stop setting this in class global and always set it above first for loop.

		resolveCollisions();
	}
	stepProgress = 1;			// Setup for next step progress.
	prevStepProgress = 1;
}
