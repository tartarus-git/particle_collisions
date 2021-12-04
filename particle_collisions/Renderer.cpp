#include "Renderer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void Renderer::render(const Scene& scene) {
	for (int i = 0; i < scene.particleCount; i++) {
		const Particle& particle = scene.particles[i];
		Ellipse(g, particle.pos.x - particle.radius, particle.pos.y - particle.radius, particle.pos.x + particle.radius, particle.pos.y + particle.radius);
	}
}
