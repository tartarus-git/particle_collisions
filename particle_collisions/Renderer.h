#pragma once

#include "Scene.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Renderer
{
public:
	HDC g;

	Renderer(const HDC g) noexcept : g(g) { }

	void render(const Scene& scene);
};

