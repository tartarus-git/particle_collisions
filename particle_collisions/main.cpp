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
		if (pos.x > windowWidth || pos.x < 0) { vel.x = -vel.x; }
		if (pos.y > windowHeight || pos.y < 0) { vel.y = -vel.y; }
		pos += vel;
	}

	void resolveCollision(Particle& other) {

		pos -= vel;
		other.pos -= other.vel;
		// a coefficient
		float a = vel.x * vel.x + other.vel.x * other.vel.x - 2 * vel.x * other.vel.x;
		a += vel.y * vel.y + other.vel.y * other.vel.y - 2 * vel.y * other.vel.y;

		// b coefficient
		float b = 2 * vel.x * pos.x - 2 * vel.x * other.pos.x - pos.x * other.vel.x * 2 + 2 * other.vel.x * other.pos.x;
		b += 2 * vel.y * pos.y - 2 * vel.y * other.pos.y - pos.y * other.vel.y * 2 + 2 * other.vel.y * other.pos.y;

		// c coefficient
		float c = pos.x * pos.x - 2 * pos.x * other.pos.x + other.pos.x * other.pos.x;
		c += pos.y * pos.y - 2 * pos.y * other.pos.y + other.pos.y * other.pos.y;
		c -= (PARTICLE_RADIUS + PARTICLE_RADIUS) * (PARTICLE_RADIUS + PARTICLE_RADIUS);

		b /= a;
		c /= a;

		float r = b * b / 4 - c;
		if (r < 0) {		// Parallel lines, deoesn't work.
			
		}
		else if (r == 0) {	// This shouldn't ever happen, don't know what it would mean.
			DebugBreak;
		}
		else {				// Two solutions.
			r = sqrt(r);
			b = -b / 2;
			float x1 = b + r;
			float x2 = b - r;
			if ((x1 > 1 || x1 < 0) && (x2 > 1 || x2 < 0)) {			// Both solutions are out of bounds of the current contex, no collision.
				pos += vel;
				other.pos += other.vel;
				return;
			}



			float actualX;
			if (x1 < x2) {											// x1 is the right one to choose.
				actualX = x1;
			}
			else {
				actualX = x2;
			}

			if (x1 > 1 || x1 < 0) { actualX = x2; }
			else if (x2 > 1 || x2 < 0) { actualX = x1; }

			if (actualX == 0) {
				pos += vel;
				other.pos += other.vel;
				return;
			}

			pos += vel * actualX;
			other.pos += other.vel * actualX;
			Vector2f normal = (pos - other.pos).normalize();
			
			Vector2f relV = vel - other.vel;
			relV = -relV;
			relV = relV.reflect(normal);
			vel += relV;

			relV = other.vel - vel;
			relV = -relV;
			relV = relV.reflect(-normal);
			other.vel += relV;

			//vel = -vel;
			//vel = vel.reflect(normal);
			//other.vel = -other.vel;
			//other.vel = other.vel.reflect(-normal);
			debuglogger::out << "collision detected!" << debuglogger::endl;
			return;
		}

		pos += vel;
		other.pos += other.vel;

	}
};

Particle a(Vector2f(200, 200), Vector2f(1, 1));
Particle b(Vector2f(400, 400), Vector2f(1.5f, 1));

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