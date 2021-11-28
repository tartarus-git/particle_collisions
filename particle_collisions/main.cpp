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

bool collisionFlag = false;

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
		if (pos.x > windowWidth || pos.x < 0) { vel.x = -vel.x; collisionFlag = false; }
		if (pos.y > windowHeight || pos.y < 0) { vel.y = -vel.y; collisionFlag = false; }
		pos += vel;
	}

	void resolveCollision(Particle& other) {

		pos -= vel;
		other.pos -= other.vel;
		// a coefficient
		double a = vel.x * vel.x + other.vel.x * other.vel.x - 2 * vel.x * other.vel.x;
		a += vel.y * vel.y + other.vel.y * other.vel.y - 2 * vel.y * other.vel.y;

		// b coefficient
		double b = 2 * vel.x * pos.x - 2 * vel.x * other.pos.x - pos.x * other.vel.x * 2 + 2 * other.vel.x * other.pos.x;
		b += 2 * vel.y * pos.y - 2 * vel.y * other.pos.y - pos.y * other.vel.y * 2 + 2 * other.vel.y * other.pos.y;

		// c coefficient
		double c = pos.x * pos.x - 2 * pos.x * other.pos.x + other.pos.x * other.pos.x;
		c += pos.y * pos.y - 2 * pos.y * other.pos.y + other.pos.y * other.pos.y;
		c -= (PARTICLE_RADIUS + PARTICLE_RADIUS) * (PARTICLE_RADIUS + PARTICLE_RADIUS);

		if (a == 0) {					// I'm pretty sure this means that the balls aren't moving RELATIVE TO EACHOTHER <-- This is because of the last term of a.

			//if (b == 0) {				// Means the distance between the balls isn't changing, which is always the case if they aren't moving relative to eachother, so they're not relatively moving and they're 1. colliding, 2. too far away, 3. too close to eachother.
				pos += vel;
				other.pos += other.vel;					// TODO: Since this triggers if the balls are ever inside eachother and not moving relatively, you might want to move them outside eachother just as a safety, even if that should never happen if you spawn the balls correctly.
				return;
			//}			<-- If statement can be commented out because a is never 0 without b being 0. Having this branch creates problems because b might not be exactly zero because rounding error and then you don't do what you're supposed to do even though you should.
		}

		b /= a;
		c /= a;

		double r = b * b / 4 - c;
		if (r < 0) {		// I assume this happens when parallel lines are too far away from eachother to produce a hit.
			
		}
		else if (r > 0 || r == 0) {					// More than one solution happens all the time. One solution happens when parallel lines are exactly r1 + r2 away from eachother and just one point is hittable.

			if (collisionFlag) { pos += vel; other.pos += other.vel; return; }

			if (r < 0) { DebugBreak(); }
			r = sqrt(r);
			b = -b / 2;
			double x1 = b + r;
			double x2 = b - r;
			if ((x1 > 1 || x1 <= 0) && (x2 > 1 || x2 <= 0)) {			// Both solutions are out of bounds of the current contex, no collision.
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

			if (x1 > 1 || x1 <= 0) { actualX = x2; }
			else if (x2 > 1 || x2 <= 0) { actualX = x1; }

			pos += vel * actualX;
			other.pos += other.vel * actualX;


			Vector2f normal = (other.pos - pos).normalize();
			Vector2f relV = ((vel * normal) * normal) - ((other.vel * normal) * normal);
			other.vel += relV;
			vel -= relV;

			collisionFlag = true;


			debuglogger::out << "collision detected!" << debuglogger::endl;
			return;
		}

		pos += vel;
		other.pos += other.vel;

	}
};

Particle a(Vector2f(200, 200), Vector2f(1, 1));
Particle b(Vector2f(400, 300), Vector2f(-1, -1));

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