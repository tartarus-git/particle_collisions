#define WINDOW_TITLE "Particle Collisions"
#define WINDOW_CLASS_NAME "particle_collisions_sim_window"

#include "windowSetup.h"

#include "debugOutput.h"

#include "OpenCLBindingsAndHelpers.h"		// TODO: Actually use OpenCL to parallelize this thing. Right now, we're just doing CPU stuff.

#define PARTICLE_MIN_DIST (PARTICLE_RADIUS * 2)
#define SQUARE_PARTICLE_MIN_DIST (PARTICLE_MIN_DIST * PARTICLE_MIN_DIST)

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

class Vector22f {
public:
	float x;
	float y;

	Vector2f() = default;
	Vector2f(float x, float y) : x(x), y(y) { }

	float calcLength() {
		return sqrt(x * x + y * y);
	}

	Vector2f normalize() {
		float length = calcLength();
		return Vector2f(x / length, y / length);
	}

	Vector2f reflect(const Vector2f& normal) {
		float dot = *this * normal;
		Vector2f thing = *this - normal * dot * 2;
		return -thing;
	}

	Vector2f& operator+=(const Vector2f& other) {				// IMPORTANT: const is super important here so that this can accept r-value refs. const lvalue ref can accept rvalue ref while lvalue ref cannot accept rvalue ref.
		x += other.x;
		y += other.y;
		return *this;
	}

	Vector2f operator-(const Vector2f& other) const {
		return Vector2f(x - other.x, y - other.y);
	}

	Vector2f& operator-=(const Vector2f& other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	float operator*(const Vector2f& other) {
		return x * other.x + y * other.y;
	}

	Vector2f operator*(float factor) const {
		Vector2f result(x * factor, y * factor);
		return result;
	}
	friend Vector2f operator*(float factor, const Vector2f& vector) { return vector * factor; }			// Implemented to take care of reverse case of the other mult function. Uses friend so that the function is accessible in global scope, which is necessary because float is the main participant now instead of Vector2f when it's this way around.

	Vector2f& operator*=(float factor) {
		x *= factor;
		y *= factor;
		return *this;
	}

	Vector2f operator-() {
		Vector2f result(-x, -y);
		return result;
	}
};

HDC g;

class Particl2e {
public:
	Vector2f pos;
	Vector2f vel;

	Particle() = default;
	Particle(Vector2f pos, Vector2f vel) : pos(pos), vel(vel) { }

	void render() {
		Ellipse(g, pos.x - PARTICLE_RADIUS, pos.y - PARTICLE_RADIUS, pos.x + PARTICLE_RADIUS, pos.y + PARTICLE_RADIUS);
	}

	void update() {
		if (pos.x > windowWidth || pos.x < 0) { vel.x = -vel.x; collisionFlag = false; }
		if (pos.y > windowHeight || pos.y < 0) { vel.y = -vel.y; collisionFlag = false; }
		pos += vel;
	}

	void resolveCollision(Particle& other) {

	}
};

void graphicsLoop() {			// TODO: Just expose this g stuff in the library so we don't have to do this boilerplate every time. Good idea or no?

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
		a.resolveCollision(b);
	}
}