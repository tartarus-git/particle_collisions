#include "Scene.h"

#include <cmath>

#include "debugOutput.h"

void Scene::loadSize(unsigned int width, unsigned int height) { this->width = width; this->height = height; }

void Scene::loadParticles(const std::vector<Particle>& particles, size_t count) { this->particles = particles; particleCount = count; lastParticle = count - 1; }
void Scene::loadParticles(const std::vector<Particle>& particles) { this->particles = particles; particleCount = particles.size(); lastParticle = particleCount - 1; }
void Scene::loadParticles(std::vector<Particle>&& particles, size_t count) { this->particles = std::move(particles); particleCount = count; lastParticle = count - 1; }
void Scene::loadParticles(std::vector<Particle>&& particles) { particleCount = particles.size(); lastParticle = particleCount - 1; this->particles = std::move(particles); }				// Order is reversed here in case the moving of one vector into another changes the size() member function return value in the temporary.

void Scene::postLoadInit() {
	lastIntersectionPartners.resize(particles.size());					// TODO: We should probably use particleCount here.
	lastIntersectionWasWithWall.resize(particles.size());
	for (size_t i = 0; i < lastIntersectionPartners.size(); i++) { lastIntersectionPartners[i] = i; lastIntersectionWasWithWall[i] = false; }
}

std::vector<size_t> invalidatedParticles;

bool Scene::resolveIntersectionWithBounds(size_t particleIndex) {
	if (lastIntersectionWasWithWall[particleIndex] && lastIntersectionPartners[particleIndex] == particleIndex) { return false; }
	Particle& particle = particles[particleIndex];

	uint32_t paddedWidth = width - particle.radius;
	if (particle.pos.x > paddedWidth) { particle.pos.x = paddedWidth; }
	else if (particle.pos.x < particle.radius) { particle.pos.x = particle.radius; }

	uint32_t paddedHeight = height - particle.radius;
	if (particle.pos.y > paddedHeight) { particle.pos.y = paddedHeight; }
	else if (particle.pos.y < particle.radius) { particle.pos.y = particle.radius; }

	lastIntersectionWasWithWall[particleIndex] = true;
	lastIntersectionPartners[particleIndex] = particleIndex;
}

bool Scene::resolveIntersectionWithParticle(size_t aIndex, size_t bIndex) {
	Particle& alpha = particles[aIndex];
	Particle& beta = particles[bIndex];

	float minDistSquared = alpha.radius + beta.radius;
	minDistSquared *= minDistSquared;
	Vector2f toAlphaFromBeta = alpha.pos - beta.pos;
	float distance = toAlphaFromBeta.getSquareLength();
	if ((lastIntersectionPartners[aIndex] != bIndex || lastIntersectionPartners[bIndex] != aIndex) && distance < minDistSquared) {
		distance = sqrt(distance);
		float adjustment = sqrt(minDistSquared) - distance;						// TODO: You should reflect the particles directly from this code block, that way, you save all the calculation required normally when resolving intersections where the particles are going towards each other.
		//adjustment /= 2;										// TODO: Stack overflow when resolving intersections is way to common, rework the system so that they don't occur. You might need to switch to loops.

		float multiplier = adjustment / (distance * (alpha.mass + beta.mass));
		alpha.pos += toAlphaFromBeta * (multiplier * beta.mass);
		beta.pos -= toAlphaFromBeta * (multiplier * alpha.mass);
		lastIntersectionPartners[aIndex] = bIndex;
		lastIntersectionPartners[bIndex] = aIndex;
		invalidatedParticles.push_back(bIndex);							// NOTE: Instead of putting this index in exactly the right spot, so that the list remains sorted (which would be O(n/2 * k) on average), we sort the list at the end (which probably gives us a better time complexity).
		resolveIntersections(bIndex);						// TODO: Will this whole recursive approach bite us in our back-side eventually, should we use loops?
		return true;
	}
	return false;
}

// SIDE-NOTE: As far as I can tell after researching a bit, CPU caches are predictive, meaning somewhere in the CPU, something is predicting which memory you're going to use next and trying to make it available to you in the CPU caches before you even need it.
// If I'm understanding that right, that means that sometimes, it's better to calculate a whole list of items instead of calculating only those items that you need to calculate. By going through the list from left to right, the predictor can fill the cache without you ever experiencing a cache miss.
// If you go through in a random access fashion, you'll get cache misses, because the predictor can't predict your moves.
// I've previously thought that the amount of cache misses doesn't matter in this case because you MUST have at least as many when going through the list from left to right, because eventually you're going to cross borders where the cache needs to fetch new data, even if you are accessing linearly.
// If that were the case, accessing a couple of values randomly would be better than accessing the full list.
// Don't get me wrong, accessing a couple of values is still better, but you need a larger overall list to justify doing it, now that linear reads apparently produce no cache misses.
// TODO: Armed with all this knowledge, a question pops up: Why doesn't there exist some sort of instruction to actively tell the caching mechanism what memory you're going to need in the next couple of units of time? That way, there need not be any prediction at all, we can just tell the CPU what we want in advance.
// Seems like that would make a lot of things way faster, there's gotta be a good reason people haven't done that yet.

void Scene::resolveIntersections(size_t particleIndex) {
	while (true) {
		bool changed = false;
		if (resolveIntersectionWithBounds(particleIndex)) { changed = true; }
		for (size_t i = 0; i < particleIndex; i++) { if (resolveIntersectionWithParticle(particleIndex, i)) { changed = true; } }
		for (size_t i = particleIndex + 1; i < particleCount; i++) { if (resolveIntersectionWithParticle(particleIndex, i)) { changed = true; } }
		if (!changed) { break; }
	}
}

// TODO: The below is a modified version of the counting sort algorithm, which works great for things whose integer representations need to be sorted.
// This is AFAIK the most efficient sorting algorithm to sort integers (at least unsigned ones, haven't thought about signed ones).
// Write some documentation in the code about what the different steps of this algorithm are and what the original algorithm looks like. Also explain the difference between the standard algorithm and the one shown in that one bookmarked blog-post.
// Also write documentation for the function as a whole of course.
void Scene::sortInvalidatedParticlesAndRemoveMultiples(size_t currentLoopIndex) {
	bool* particleCounts = new bool[particleCount];
	for (size_t i = 0; i < particleCount; i++) { particleCounts[i] = false; }
	for (size_t i = 0; i < invalidatedParticles.size(); i++) { particleCounts[invalidatedParticles[i]] = true; }
	size_t sortedInvalidatedParticlesIndex = 0;
	for (size_t i = 0; i < particleCount; i++) { if (i >= currentLoopIndex) { break; } if (particleCounts[i]) { invalidatedParticles[sortedInvalidatedParticlesIndex++] = i; } }
	delete[] particleCounts;
	invalidatedParticles.resize(sortedInvalidatedParticlesIndex);
}

/*void Scene::findCollision(size_t aIndex, size_t bIndex, const Vector2f& remainingAlphaVel) {
	// TODO: Implement raw collision calculation, that just finds the t value and checks if this is the most relevant collision, same as other one, just without the guard code, because that can't be useful at this stage.
}*/

void Scene::recalculateInvalidatedData(size_t currentLoopIndex) {
	sortInvalidatedParticlesAndRemoveMultiples(currentLoopIndex);
	Vector2f remainingAlphaVel;
	size_t previousNormalParticleIndex = 0;
	for (size_t invalidatedParticleIndex = 0; invalidatedParticleIndex < invalidatedParticles.size(); invalidatedParticleIndex++)
	{
		remainingAlphaVel = particles[invalidatedParticles[invalidatedParticleIndex]].vel * currentSubStep;
		findWallCollision(invalidatedParticleIndex, remainingAlphaVel);
		for (size_t previousInvalidatedParticleIndex = 0; previousInvalidatedParticleIndex < invalidatedParticleIndex; previousInvalidatedParticleIndex++)
		{						// TODO: using iterators here might even be more efficient, check those out and see if they're applicable here.
			for (; previousNormalParticleIndex < invalidatedParticles[previousInvalidatedParticleIndex]; previousNormalParticleIndex++)
			{
				findCollision(invalidatedParticles[invalidatedParticleIndex], previousNormalParticleIndex, remainingAlphaVel);
			}
			previousNormalParticleIndex++;
		}
		for (; previousNormalParticleIndex < invalidatedParticles[invalidatedParticleIndex]; previousNormalParticleIndex++)
		{
			findCollision(invalidatedParticles[invalidatedParticleIndex], previousNormalParticleIndex, remainingAlphaVel);
		}
		previousNormalParticleIndex++;
		for (; previousNormalParticleIndex < particleCount; previousNormalParticleIndex++)
		{
			findCollision(invalidatedParticles[invalidatedParticleIndex], previousNormalParticleIndex, remainingAlphaVel);
		}
	}
}

void Scene::findWallCollision(size_t index, const Vector2f& remainingVel) {
	Particle& particle = particles[index];

	float paddedWidth = width - particle.radius;					// TODO: This paddedBound stuff can be easily cached. You should make a very simple system of functions that handle the various caches that you're gonna end up having. To make sure they get updated at the right time.
	float paddedHeight = height - particle.radius;					// TODO: I'm very sure that storing an array of cached padded bounds for each particle (since they all can be differently sized) would not make this more efficient. The amount of instructions stays the same AFAIK. Can't see how it would help.

	Vector2f futurePos = particle.pos + remainingVel;

	// TODO: Find a way to clean up the next bit of code, even if it's just putting it on separate lines.
	if (futurePos.x > paddedWidth) { float t = (paddedWidth - particle.pos.x) / remainingVel.x; if (t < lowestT) { lowestT = t < 0 ? 0 : t; noCollisions = false; boundsCollision = true; currentColliderA = index; currentColliderB = false; return; } }
	else if (futurePos.x < particle.radius) { float t = (particle.radius - particle.pos.x) / remainingVel.x; if (t < lowestT) { lowestT = t < 0 ? 0 : t; noCollisions = false; boundsCollision = true; currentColliderA = index; currentColliderB = false; return; } }

	if (futurePos.y > paddedHeight) { float t = (paddedHeight - particle.pos.y) / remainingVel.y; if (t < lowestT) { lowestT = t < 0 ? 0 : t; noCollisions = false; boundsCollision = true; currentColliderA = index; currentColliderB = true; return; } }
	else if (futurePos.y < particle.radius) { float t = (particle.radius - particle.pos.y) / remainingVel.y; if (t < lowestT) { lowestT = t < 0 ? 0 : t; noCollisions = false; boundsCollision = true; currentColliderA = index; currentColliderB = true; return; } }

	return;


	// NOTE: If a particle is outside the bounds of the scene, the position is corrected and the particle is reflected (much like the particle intersection safeguard at the beginning of findCollision function).
	// One could do some things (which would be efficient and would technically make the program more efficient) in the case of an out of bounds particle, that would cause the control flow to skip the evaluation of most of the rest of the particle pairs.
	// (The same optimization could be applied to the particle intersection guard code in the findCollision function.)
	// While this would make the program run faster, it would only do so on the very rare occasion that a particle actually goes out of bounds (the algorithm protects against that). Theres no overhead in implementing the optimization, so one would think that we should do it, but its harmful in one specific situation:
	// If not only one particle goes out of bounds, but a bunch of particles do and a bunch of other particles suddenly start intersecting with each other (idk why this would happen, but if something crazy goes wrong), it would take way more substeps to correct this situation than if we were to not implement the aforementioned
	// optimzation. This is super super super unlikely, but if it does happen, I think it's more important to recover from that situation quickly than to recover from single particles outside of bounds quickly. The reason I think this is because recovering from single particle issues is already super quick, even without the
	// optimzation. By putting in the optimization, we would be sacrificing a hugely better recovery time for one thing for a marginally better recovery time on another thing.
	// BTW, the optimization would be to pass index by reference (this function is very likely to be inlined, so reference would (I assume) be free) and then set it equal to lastParticle - 1 when we detect an out of bounds particle. This would skip a bunch of technically unnecessary calculation, but like I said,
	// we're going to do the unnecessary calculations because that causes the intersection guard in findCollisions to be hit and this out of bounds guard to be hit for all the particles, which causes way more errors to be corrected in one substep.
	// BTW, the calculations are unnecessary because once we set lowestT to 0 here, no other calculation will change the value again, they do nothing for us, hence, useless (like I said, theoretically).
	// TODO: Could we just do the optimization but then start a loop in this function to go through everything and just check the guards? That would be the best of both worlds and be super efficient.

	if (particle.pos.x < particle.radius) {
		particle.pos.x = particle.radius;
		// TODO: When we get a hit in one of these buckets, do the following: go through all the upcoming particles from inside this function and make sure all of their intersection guard code gets a chance to run. Then, set the i and j member variables to a state that almost resembles the start of the main while loop
		// (your not gonna be able to get the main while loop start simulated perfectly, but you'll get close), then set lowestT to 1 and noCollisions to true, then do the first particles calculations (wall hits and stuff, because the for loop isn't going to reach that even with i and j set to 0).
		// What this does is this: with no overhead, you've created a situation where, after returning from this function, it'll be as if the next substep has started, which it essentially has. This will forego a bunch of unnecessary processing of all sorts of stuff (unnecessary because of intersection and because lowestT is 0
		// when an intersection happens), but most importantly, it'll forgo calling the reflectCollision function, which means you won't have to do any branching to find out whether there are particles you need to reflect or not.
		// Just as a reminder, the plan is also the do the reflections directly in this function when the need arises (only when doing out of bounds intersection resolution), and also in the intersection guard in the findCollision function.
		// This will allow all of the intersections of the substep to be resolved and reflected without having to start new substeps in between because the collision reflector has to run. The collision reflector does one pair at a time, which we don't have to abide by if we forego it in these specific circumstances.
	}
	else { unsigned int xBoundary = width - particle.radius; if (particle.pos.x > xBoundary) { particle.pos.x = xBoundary; } }
	// NOTE: You might consider caching width - particle.radius for every particle to make the above faster, but indexing into the resulting array would be less efficient than the current setup (because the array would be in heap), don't do it.
	if (particle.pos.y < particle.radius) { particle.pos.y = particle.radius; }
	else { unsigned int yBoundary = height - particle.radius; if (particle.pos.y > yBoundary) { particle.pos.y = yBoundary; } }
}

void Scene::findCollision(size_t aIndex, size_t bIndex, const Vector2f& remainingAlphaVel) {
	Particle& alpha = particles[aIndex];
	Particle& beta = particles[bIndex];

	if (bIndex == lastParticle) {
		//debuglogger::out << "bruh" << debuglogger::endl;
	}

	// If the two particles are inside each other (which shouldn't ever happen unless they are spawned wrong or their positions are changed from outside of the simulation), move them outside of each other using the shortest possible path.
	float minDist = alpha.radius + beta.radius;				// TODO: This should be moved to the top, two particles can still intersect even though they just hit each other if some weird outside forces are applied, this safety feature needs to be at the top.
	Vector2f toAlphaFromBeta = alpha.pos - beta.pos;
	//float distance = toAlphaFromBeta.getLength();
	/*if (distance < minDist) {						// TODO: See about getting not only one of these intersections reflected per run. Maybe store currentColliders in a two vectors so that you can massively reflect stuff when this happens. But the overhead probably isn't worth it, think through it a couple times.
		float adjustment = minDist - distance;						// TODO: Instead of doing that, just reflect the particles directly in this code block, that would be an awesome solution, somehow, your going to need to be able to tell the reflector to do nothing though. Too much overhead?
		toAlphaFromBeta *= adjustment / distance;
		alpha.pos += toAlphaFromBeta;
		beta.pos -= toAlphaFromBeta;
		//lowestT = 0;															// If we resolve the collision without marking it as a collision, the two particles might go into each other again on the next frame. We need the reflection code to reflect the particles off of one another so they move in different directions.
		//currentColliderA = aIndex;												// NOTE: Technically, if the velocities are the same, there is no reflection to be done, but spending an if statement on something that probably isn't going to happen often at all is inefficient.
		//currentColliderB = bIndex;		// NOTE: Marking this as collision is also super important because if we didn't, and beta (for example) was already marked as current collider this round, the coming collision would be totally off because we move beta here. We HAVE to mark this as collision to avoid that situation.
		//noCollisions = false;			// Since we move both particles in this intersection resolution code, even if the resolution moves one particle into another intersection situation, the system should naturally resolve everything over the course of the next few rounds (NOT frames). That is beautiful.
		//return;
	}*/

	// This chunk of code is super important, it prevents floating point inaccuracies from messing up the simulation and as a bonus, prevents objects from sticking together after some other error causes them to intersect instead of reflect.
	// The need for this chunk comes from the fact that, after moving two colliding particles right up against each other and reflecting their velocities, because of floating point error, they will probably still count as colliding in the next simulation round, which we don't want.
	// The original fix for this was to keep track of each particles last collision partner, since (because of the nature of physics) you can't collide with the same partner twice in a row. This works fine until the user wants to add additional forces into the simulation (like gravity for example).
	// When that happens, it actually is possible to collide with the same particle twice in a row, making the system useless. Now, we have a slightly more expensive system which doesn't have any of the faults of the previous system.
	// It's very simple, if a particle and it's partner are moving away from each other, there is no way they are going to collide, so don't bother checking anything and just exit.
	// This filters out situations where the velocities of two particles are reflected but they are still right next to each other.
	// The afore-mentioned bonus comes from the fact that, if two particles end up inside each other but are moving in any directions that aren't the same, this will count as a movement away from each other, which causes the system to allow the particles to cleanly seperate without triggering any collision code.
	// This should technically never happen in normal operation, but it's nice that the system is forgiving in that regard.
	// NOTE: if alphaVelTowardsComp and betaVelTowardsComp are equal, that doesn't mean that the particles have the same velocity. Those two variables can be equal even when the particles are moving away from each other super fast.
	// This is because we're just looking at the dot product shadows of the vectors on the distDirNorm. As long as the shadows indicate an equivalent value, the particles can move however far they want in a perpendicular direction from the distDirNorm.
	// This isn't an issue though, because we want to filter out the moving apart particles anyway. The equivalent case also happens when the velocities are the same, so we can still filter that out as well, so everythings fine, it's just a little unexpected.
	Vector2f distDirNorm = toAlphaFromBeta.normalize();
	float alphaVelTowardsComp = alpha.vel % distDirNorm;
	float betaVelTowardsComp = beta.vel % distDirNorm;
	if (alphaVelTowardsComp >= betaVelTowardsComp) { return; }

	Vector2f remainingBetaVel = beta.vel * currentSubStep;

	// Construct coefficients necessary for solving the quadratic equation the describes particle collisions.

	// a coefficient
	Vector2f velDiff = remainingAlphaVel - remainingBetaVel;
	float a = velDiff % velDiff;
	// NOTE: We previously did "if (a == 0) { return; }" here, because we needed to protect ourselves from divide by zero errors in the following code.
	// We don't do that anymore because for a to be zero, the particles have to be not moving relative to each other (velocities pointing in exactly the same direction), which the above guard code with the distDirNorm stuff filters out for us already. a == 0 is not possible, so we don't need the check anymore.
	// I initially worried that floating point error could still cause a to be zero even if the guard code doesn't filter anything out, but I've gone through the code and it looks impossible. I'm very sure that it's completely safe, so no reason to use a == 0 check here.

	// b coefficient (additionally, I've divided by 2a from the get-go so that it fits nicely into a more efficient version of the quadratic equation)
	// NOTE: After reading this, you might think that I've forgotten to divide by 2, but I did it by removing a multiplication that was supposed to be there, so everythings fine, don't worry.
	float b = (velDiff % toAlphaFromBeta) / a;

	// c coefficient (I've divided this one by a from the get-go as well for the same reason)
	float c = ((toAlphaFromBeta % toAlphaFromBeta) - minDist * minDist) / a;							// TODO: Try combining b with c here and seeing if the amount of operations is lower. You still have to keep a raw b around for later though, so it probably won't be efficient in the grand scheme of things to combine b and c here, but do the math and check. You might be able to avoid a division, but probably not.
	// TODO: C isn't used anywhere except r, so when combining b and c, you can combine them directly into r and save a variable.

	// Constructing the determinant.
	float r = b * b - c;

	// If no collisions (because trajectories are parallel and too far away from each other for a parallel (head on) collision), return. This can also happen when one or both of the particles aren't moving.
	if (r < 0) { return; }

	// The following path gets taken if 1 possible collision (parallel lines that are exactly the right distance away), or 2 possible collisions (all other non-handled collisions).
	// It's inefficient to have separate branch for 1 collision because it happens so rarely, we just handle it through the math of 2 collision handler.

	// NOTE: Before we move on, I'm going to explain exactly what these two different collision possiblities represent:
	// We're defining a collision as the moment in time where two particles just barely touch (their distance is equal to their summed radii). This happens if they collide, but also if they were to move through each other and touch each others backs just before leaving each others influence.
	// These points are both on the particle's trajectories and the formula picks up both of them. The following code filters out the "collision" that happens the latest, because that one has to be the invalid one (because you have to go into each other before you can go back out).

	r = sqrt(r);											// Now that we know that the determinant is non-negative, it's safe to do the square root.

	b = -b;													// Construct the final p term, we didn't do this when we originally created it (even though it is theoretically possible and also clean) because there is no need to do extra calculation when we might quit because of r < 0 anyway. Now that we definitely didn't quit, it's ok.

	float t1 = b + r;										// Calculate both possible collisions.
	float t2 = b - r;

	// Now to find the collision among the two posibilities.
	// NOTE: Both possibilities can also be wrong. The reason is because the formula detects collisions along our infinitely long trajectories, but only if the collisions happen in the next frame do they matter to us.
	// TODO: This actually presents an opportunity for a massive optimization. We can find the lowest t-value, but we can still count it even if it is super high (for example 200). Then we initialize a counter at 200 and for the next 200 frames, we know that no particle in the simulation will collide with anything.
	// This presents a problem in case the simulation changes somehow while we're doing our "cached frames", like if after 100 frames, a particle appears out of no where, we're not going to be able to collide with it.
	// To fix this issue, have some sort of clear cache function that any other code will have to call if it changes the simulation on a wim. This will set our counter to 0 or something, and will tell the scene code that it does need to start calculating the physics properly again.
	// This could improve the performance by so so so so so much when the particles aren't currently colliding with each other. Even if it's just active for a couple of frames at a time, this could make the simulation so much faster.

	// NOTE: According to the IEEE standard, which pretty much all popular C++ compilers stick to AFAIK, float comparisons between a NaN and something else (even if that something else is also a NaN) always equate to false. This means that f != f returns true when f = NaN.
	// This cannot be relied on if you're planning on having your code work even when compiled using the -ffast-math flag. This flag strays from the IEEE standard to allow some optimizations to work, causing stuff f != f to be unreliable.
	// To be compatible with -ffast-math, one should probably just avoid NaN alltogether, which is what we're doing in this class by checking if a == 0 before moving on (see above). (In case you don't understand: dividing by 0 causes NaN, which we thereby avoid)
	// Another way to check for NaN is to check the bit pattern of the float at hand, which should work with -ffast-math in most cases, but if -ffast-math somehow changes the bit pattern for certain floats (because it doesn't have to stick to IEEE), then this is unreliable as well.

	// The following code looks for the lowest of the two t-values and checks if that value is lower than lowestT. If it is, lowestT is set to that value and this function exits.
	// Before checking against lowestT, the code will check if the t-value is less than 0, meaning it is invalid. If this is the case, but the other t-value is above 0, that means that the two particles are intersecting as of t=0. This is almost always due to floating point error, meaning it's not perceptible to the eye.
	// We don't bother moving the particles outside of each other since it's a very small error and resolving it could actually introduce a new error of the same sort. Resolving it could also create more jitter in the case where particles are packed tightly against each other, which isn't optimal.
	// We handle this case by setting lowestT to 0 and signalling a collision, after which we return. This collides the two particles and allows them to bounce against each other as if they were only touching, which they essentially are, there is pretty much nothing we can do about the floating point error.
	// NOTE: BTW, the above situation can happen (for example) when a particle reflects but gets reflected back by another particle before it has a chance to follow it's reflected velocity. When this happens, it'll be moving towards the original reflection partner while still being inside of it, causing it to not
	// be filtered out by the guard code (not that we want it to be). This is why we need the extra cases here, so that we can overcome this situation, which is solely caused by floating point error making the two particles be inside each other a little bit.

	if (t1 < t2) {
		if (t1 < 0) {
			if (t2 > 0) { lowestT = 0; currentColliderA = aIndex; currentColliderB = bIndex; noCollisions = false; boundsCollision = false; return; }			// NOTE: It is not possible for t2 to equal 0 when t1 is less than 0 because the particles have to be moving towards each other at this stage,
			return;																																				// which is why we don't need to check for it, even though it looks like we should.
		}
		if (t1 < lowestT) { lowestT = t1; currentColliderA = aIndex; currentColliderB = bIndex; noCollisions = false; boundsCollision = false; return; }
	}
	else {
		if (t2 < 0) {
			if (t1 > 0) { lowestT = 0; currentColliderA = aIndex; currentColliderB = bIndex; noCollisions = false; boundsCollision = false; return; }
			return;
		}
		if (t2 < lowestT) { lowestT = t2; currentColliderA = aIndex; currentColliderB = bIndex; noCollisions = false; boundsCollision = false; return; }
	}
}

void Scene::reflectCollision() {
		Particle& alpha = particles[currentColliderA];
	if (boundsCollision) {
		if (currentColliderB) {
			alpha.vel.y = -alpha.vel.y;
			return;
		}
		alpha.vel.x = -alpha.vel.x;
		return;
	}

		Particle& beta = particles[currentColliderB];
		Vector2f normal = (beta.pos - alpha.pos).normalize();								// TODO: Caches these because you calculate them for every pair anyway in the guard code for findCollision.
		Vector2f relV = ((alpha.vel % normal) * normal) - ((beta.vel % normal) * normal);			// TODO: This can be algebraically optimized.
		alpha.vel -= relV;
		beta.vel += relV;
}

// TODO: Currently, we are checking for intersections for every particle pair in every sub-step. It would be way more efficient to check all the intersections in the first sub-step, but not in the rest.
// If the first substep ends up in a valid state, all the permutations to that state that we do in the next state don't cause any intersections.
// The only way intersections can happen is through external position changes between STEPS. It is not possible for outside pos changes to come in while we are substepping, making our guard code partially useless.
// This is of course only if there isn't another thread messing with our stuff, but that would probably cause other problems as well and that isn't expected anyway, so don't worry about it.
// Also, bit-flips due to cosmic rays and other radiation could definitely mess up our positions while we are substepping, but we can never know if a position discrepancy is due to floating point error or due to intersections without using a window, which we don't want to do because it's not realistic.
// That means, that even if we were to use guard code every sub-step, we would still be just as vulnerable to bit-flips. There is no reason not to make this more efficient by moving the guard code outside of the substep loop.

void Scene::step() {
	currentSubStep = 1;
	while (true) {
		lowestT = 1;
		noCollisions = true;
		invalidatedParticles.clear();
		for (int i = 0; i < lastParticle - 1; i++) {
																// TODO: When resolving intersections, make the mass play a role, so that massive objects get moved less and the smaller ones pick up more of the slack. It'll pull the most realism out of the process. This is super important because it'll look way better.
			if (particles[i].lastInteractionWasIntersection) { resolveIntersections(i); recalculateInvalidatedData(i); particles[i].lastInteractionWasIntersection = false; }
			Vector2f remainingAlphaVel = particles[i].vel * currentSubStep;
			findWallCollision(i, remainingAlphaVel);
			for (int j = i + 1; j < particleCount; j++) {				// TODO: For loop does first iteration before checking right? If it doesn't that is unnecessary work here.
				findCollision(i, j, remainingAlphaVel);									// NOTE: If a weird intersection happens, then lowestT might be zero before we get to the end of these loops. We could do an if statement to exit prematurely in that case, but the chances of it happening are too low. It would be inefficient to waste time checking that case.
			}
		}

			if (particles[lastParticle - 1].lastInteractionWasIntersection) { resolveIntersections(lastParticle - 1); recalculateInvalidatedData(lastParticle - 1); particles[lastParticle - 1].lastInteractionWasIntersection = false; }
			Vector2f remainingAlphaVel = particles[lastParticle - 1].vel * currentSubStep;
			findWallCollision(lastParticle - 1, remainingAlphaVel);
			if (particles[lastParticle].lastInteractionWasIntersection) { resolveIntersections(lastParticle); recalculateInvalidatedData(lastParticle); particles[lastParticle].lastInteractionWasIntersection = false; }
			else { findCollision(lastParticle - 1, lastParticle, remainingAlphaVel); }									// NOTE: If a weird intersection happens, then lowestT might be zero before we get to the end of these loops. We could do an if statement to exit prematurely in that case, but the chances of it happening are too low. It would be inefficient to waste time checking that case.
			findWallCollision(lastParticle, particles[lastParticle].vel * currentSubStep);
		if (noCollisions) { break; }
		float subStepProgress = currentSubStep * lowestT;						// Store the fraction of the current substep that every particle can now safely put behind itself.

		for (int i = 0; i < particleCount; i++) {
			Particle& particle = particles[i];
			particle.pos += particle.vel * subStepProgress;
		}
		reflectCollision();

		currentSubStep -= subStepProgress;										// Set the next substep to be equal to the fraction of the current substep that we haven't traversed yet.
	}
	for (int i = 0; i < particleCount; i++) {
		particles[i].pos += particles[i].vel * currentSubStep;
	}
}
