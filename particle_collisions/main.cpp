#define PARTICLE_RADIUS 50

#define WINDOW_TITLE "Particle Collisions"

#define WINDOW_CLASS_NAME "particle_collisions_sim_window"
#include "windowSetup.h"

#include "debugOutput.h"

#include "OpenCLBindingsAndHelpers.h"		// TODO: Actually use OpenCL to parallelize this thing. Right now, we're just doing CPU stuff.

unsigned int windowWidth;
unsigned int windowHeight;

void setWindowPos(int newPosX, int newPosY) {

}

void setWindowSize(unsigned int newWidth, unsigned int newHeight) {
	windowWidth = newWidth;
	windowHeight = newHeight;
}

void setWindow(int newPosX, int newPosY, unsigned int newWidth, unsigned int newHeight) {
	setWindowPos(newPosX, newPosY);
	setWindowSize(newWidth, newHeight);
}

LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (listenForExitAttempts(uMsg, wParam, lParam)) { return 0; }
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

class Vector2f {
public:
	float x;
	float y;

	Vector2f() = default;
	Vector2f(float x, float y) : x(x), y(y) { }

	/*Vector2f& operator+(Vector2f& other) {				// Reference is ok in all situations in this case because we don't change the value, which allows the compiler to avoid using pointers if the function can't be inlined.
		Vector2f result;
		result.x = x + other.x;
		result.y = y + other.y;
		return result;
	}*/

	Vector2f& operator+=(Vector2f& other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	Vector2f& operator*=(float factor) {
		x *= factor;
		y *= factor;
		return *this;
	}
};

HDC g;

class Particle {
public:
	Vector2f pos;
	Vector2f vel;

	Particle() = default;
	Particle(Vector2f pos, Vector2f vel) : pos(pos), vel(vel) { }

	void render() {
		Ellipse(g, pos.x - PARTICLE_RADIUS, pos.y - PARTICLE_RADIUS, pos.x + PARTICLE_RADIUS, pos.y + PARTICLE_RADIUS);
	}

	void update() {
		pos += vel;
		if (pos.x > windowWidth || pos.x < 0) { vel.x = -vel.x; }
		if (pos.y > windowHeight || pos.y < 0) { vel.y = -vel.y; }
	}

	void resolveCollision(Particle other) {

	}
};

Particle a(Vector2f(200, 200), Vector2f(1, 1));
Particle b(Vector2f(300, 300), Vector2f(2, 1));

void graphicsLoop() {			// TODO: Just expose this g stuff in the library so we don't have to do this boilerplate every time. Good idea or no?

	a.vel *= 0.4f;
	b.vel *= 0.4f;

	HDC finalG = GetDC(hWnd);
	g = CreateCompatibleDC(finalG);
	HBITMAP bmp = CreateCompatibleBitmap(finalG, windowWidth, windowHeight);
	SelectObject(g, bmp);

	HPEN particlePen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
	HBRUSH particleBrush = CreateSolidBrush(RGB(0, 255, 0));

	HPEN bgPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	HBRUSH bgBrush = CreateSolidBrush(RGB(0, 0, 0));

	while (isAlive) {
		SelectObject(g, bgPen);
		SelectObject(g, bgBrush);
		Rectangle(g, 0, 0, windowWidth, windowHeight);
		SelectObject(g, particlePen);
		SelectObject(g, particleBrush);
		a.render();
		b.render();
		BitBlt(finalG, 0, 0, windowWidth, windowHeight, g, 0, 0, SRCCOPY);
		a.update();
		b.update();
		a.resolveCollision(b);
	}
}