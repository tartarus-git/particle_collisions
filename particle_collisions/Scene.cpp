#include "Scene.h"

void Scene::setupParticleHelpers() {
	lastParticleCollisions.resize(particleCount);
	remainingVels.resize(particleCount);
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
	if (lastParticleCollisions[aIndex] == bIndex) { return; }

	Particle& alphaRef = particles[aIndex];
	Particle& betaRef = particles[bIndex];
	Particle alpha = alphaRef;			// TODO: See if you should do this or not. Potentially compare assembly. Ask on stackoverflow as well maybe.
	Particle beta = betaRef;

	Vector2f posDiff = alpha.pos - beta.pos;
	float minDist = alpha.radius + beta.radius;
	float squareMinDist = minDist * minDist;
	Vector2f specialVelDiff = alpha.vel - 2 * beta.vel;
	// a and b will equal zero if particles have same velocity and either aren't colliding or are colliding (the latter should theoretically never happen unless the particles get spawned wrong).
	if (specialVelDiff.isZero()) {
		float length = posDiff.getLength();
		float dist = length - squareMinDist;
		if (dist < 0) {								// If they ARE somehow inside each other, fix the collision straight away since there is nothing to simulate back to.
			posDiff *= dist / length;
			betaRef.pos += posDiff;
			alphaRef.pos -= posDiff;
		}
		return;
	}

	// a coefficient
	float a = (alpha.vel * specialVelDiff + beta.vel * beta.vel).sum();

	// b coefficient
	float b = (2 * posDiff * (alpha.vel - beta.vel)).sum();

	// c coefficient
	float c = (alpha.pos * (alpha.pos - 2 * beta.pos) + beta.pos * beta.pos).sum();
	// make one side equal to zero
	c -= squareMinDist;

	// Reduce a coefficient to 1.
	b /= 2 * a;				// Simultaniously construct sudo p term.
	c /= a;

	// Construct intra-root term.
	float r = b * b - c;

	// If no collisions (because parallel trajectories that are too far away from each other for a parallel collision), return.
	// We don't put 1 into earliestCollisions list here because the default value in earliestCollisions is 1. We don't have to do anything.
	if (r < 0) { return; }

	// Following gets triggered if 1 collision (parallel lines that are exactly the right distance away), or 2 collisions (all other non-handled collisions).
	// Inefficient to have separate branch for 1 collision because it happens so rarely, just handle it through the math of 2 collision handler.
	r = sqrt(r);
	b = -b;					// Construct full p term.
	float t1 = b + r;		// Calculate both possible collisions.
	float t2 = b - r;
	float actualT;			// Find the actual collision.
	if (t1 > 1 || t1 <= 0) {
		if (t2 > 1 || t2 <= 0) { return; }							// Both solutions are out of bounds. No collision.
		actualT = t2;												// x2 is the correct solution.
	} else if (t2 > 1 || t2 <= 0) { actualT = t1; }					// x1 is the correct solution.
	else { if (t1 < t2) { actualT = t1; } else { actualT = t2; } }	// The lower of the two solutions if the correct solution. This is because the collision that happens the earliest is the real one.
	
	// Store this collision timepoint if it is earlier than the previous earliest timepoint.
	float realTA = actualT * (1 - earliestCollisions[aIndex]) + earliestCollisions[aIndex];
	float realTB = actualT * (1 - earliestCollisions[bIndex]) + earliestCollisions[bIndex];
	if (realTA < earliestCollisions[aIndex]) { earliestCollisions[aIndex] = realTA; lastParticleCollisions[aIndex] = bIndex; }
	if (realTB < earliestCollisions[bIndex]) { earliestCollisions[bIndex] = realTB; lastParticleCollisions[bIndex] = bIndex; }
	// TODO: The earliestCollisions thing works, it just can't be the backbone of recursion here. You have to start caching new vels or something.



			/*Vector2f normal = (other.pos - pos).normalize();
			Vector2f relV = ((vel * normal) * normal) - ((other.vel * normal) * normal);
			other.vel += relV;
			vel -= relV;

			collisionFlag = true;


			debuglogger::out << "collision detected!" << debuglogger::endl;
			return;*/
	return;
}

void Scene::findSubCollisions(size_t aIndex, size_t bIndex) {
	if (lastParticleCollisions[aIndex] == bIndex) { return; }

	Vector2f& aPos = particles[aIndex].pos;
	Vector2f& bPos = particles[bIndex].pos;
	Vector2f& aRemVel = remainingVels[aIndex];
	Vector2f& bRemVel = remainingVels[bIndex];

	Vector2f posDiff = alpha.pos - beta.pos;
	float minDist = alpha.radius + beta.radius;
	float squareMinDist = minDist * minDist;
	Vector2f specialVelDiff = alpha.vel - 2 * beta.vel;
	// a and b will equal zero if particles have same velocity and either aren't colliding or are colliding (the latter should theoretically never happen unless the particles get spawned wrong).
	if (specialVelDiff.isZero()) {
		float length = posDiff.getLength();
		float dist = length - squareMinDist;
		if (dist < 0) {								// If they ARE somehow inside each other, fix the collision straight away since there is nothing to simulate back to.
			posDiff *= dist / length;
			betaRef.pos += posDiff;
			alphaRef.pos -= posDiff;
		}
		return;
	}

	// a coefficient
	float a = (alpha.vel * specialVelDiff + beta.vel * beta.vel).sum();

	// b coefficient
	float b = (2 * posDiff * (alpha.vel - beta.vel)).sum();

	// c coefficient
	float c = (alpha.pos * (alpha.pos - 2 * beta.pos) + beta.pos * beta.pos).sum();
	// make one side equal to zero
	c -= squareMinDist;

	// Reduce a coefficient to 1.
	b /= 2 * a;				// Simultaniously construct sudo p term.
	c /= a;

	// Construct intra-root term.
	float r = b * b - c;

	// If no collisions (because parallel trajectories that are too far away from each other for a parallel collision), return.
	// We don't put 1 into earliestCollisions list here because the default value in earliestCollisions is 1. We don't have to do anything.
	if (r < 0) { return; }

	// Following gets triggered if 1 collision (parallel lines that are exactly the right distance away), or 2 collisions (all other non-handled collisions).
	// Inefficient to have separate branch for 1 collision because it happens so rarely, just handle it through the math of 2 collision handler.
	r = sqrt(r);
	b = -b;					// Construct full p term.
	float t1 = b + r;		// Calculate both possible collisions.
	float t2 = b - r;
	float actualT;			// Find the actual collision.
	if (t1 > 1 || t1 <= 0) {
		if (t2 > 1 || t2 <= 0) { return; }							// Both solutions are out of bounds. No collision.
		actualT = t2;												// x2 is the correct solution.
	} else if (t2 > 1 || t2 <= 0) { actualT = t1; }					// x1 is the correct solution.
	else { if (t1 < t2) { actualT = t1; } else { actualT = t2; } }	// The lower of the two solutions if the correct solution. This is because the collision that happens the earliest is the real one.
	
	// Store this collision timepoint if it is earlier than the previous earliest timepoint.
	float realTA = actualT * (1 - earliestCollisions[aIndex]) + earliestCollisions[aIndex];
	float realTB = actualT * (1 - earliestCollisions[bIndex]) + earliestCollisions[bIndex];
	if (realTA < earliestCollisions[aIndex]) { earliestCollisions[aIndex] = realTA; lastParticleCollisions[aIndex] = bIndex; }
	if (realTB < earliestCollisions[bIndex]) { earliestCollisions[bIndex] = realTB; lastParticleCollisions[bIndex] = bIndex; }
	// TODO: The earliestCollisions thing works, it just can't be the backbone of recursion here. You have to start caching new vels or something.



			/*Vector2f normal = (other.pos - pos).normalize();
			Vector2f relV = ((vel * normal) * normal) - ((other.vel * normal) * normal);
			other.vel += relV;
			vel -= relV;

			collisionFlag = true;


			debuglogger::out << "collision detected!" << debuglogger::endl;
			return;*/
	return;
}

void Scene::resolveCollisions() {
	for (int i = 0; i < particleCount; i++) {
		Particle& particle = particles[i];
		particle.pos += particle.vel * earliestCollisions[i];
		if (lastParticleCollisions[i] == i) { continue; }		// This means that the collision was already recorded through another particle's index, so we're good.
		Particle& other = particles[lastParticleCollisions[i]];
		Vector2f normal = (other.pos - particle.pos).normalize();
		Vector2f relV = ((particle.vel % normal) * normal) - ((other.vel % normal) * normal);
		particle.vel -= relV;
		other.vel += relV;
	}
}

void Scene::step() {
	// Find the earliest safepoint of all the particles.
	for (int i = 0; i < lastParticle; i++) {
		for (int j = i; j < particleCount; j++) {

			findCollision(i, j);
		}
	}

	for (int i = 0; i < )

	resolveCollisions();
	step();
}
