#define WINDOW_TITLE "Particle Collisions"
#define WINDOW_CLASS_NAME "particle_collisions_sim_window"

#include "windowSetup.h"

#include "debugOutput.h"

#include "OpenCLBindingsAndHelpers.h"		// TODO: Actually use OpenCL to parallelize this thing. Right now, we're just doing CPU stuff.

#include "Scene.h"

#include <cstdlib>

#include "Renderer.h"

// NOTE: Remember the -ffast-math flag for future use. It makes your math faster by having it lose precision in some places, which isn't a problem for a lot of use cases. We probably don't want it here because we want accurate physics simulation, but just keep it in mind for future use.
// NOTE: -ffast-math is the gcc flag I believe. It's probably called something else for MSVC. You can almost definitely check some sort of box in the configuration menu for this project or something.
// TODO: Also, research all of the optimizations that fast math does and understand them because those might be interesting.

// SIDE-NOTE THAT I'M WRITING DOWN TO REMEMBER IT IN FUTURE, HAS VERY LITTLE TO DO WITH THIS CODEBASE:
// So I've been vastly underestimating the parallelism of modern processors. I thought that each core basically just does one instruction after another, which is completely wrong. Thats also the reason you can't really say how many cycles an instruction will take, because it varies depending on the
// conditions of the processor at the current time (what other operations it is doing, how busy each of it's parts are and such). You can however say how many cycles an instruction takes on average (often called throughput AFAIK) or how much time an instruction takes. A useful way of measuring stuff like that is
// to run millions of the same instruction and then divide the total amount of time used by the amount of instructions that were run, it's like measuring how many particles are in a cup, you use mol and such (that might not be a completely apt comparison and I'm not a chemistry expert so idk).
// There are a couple of processer design principles that cause this parallelism, like pipelining, which splits the execution of an instruction across multiple stages (each taking a clock cycle usually) but fetches a new instruction every cycle, thereby executing multiple instructions in parallel but offset by one stage from one another.
// This doesn't actually increase the speed in and of itself, it just increases processor latency, which is actually bad technically. The speed increase that comes from pipelining comes from the fact that, since less logic is being done in series per cycle, the clock isn't required to wait as long for the transistors to flip and for stuff to happen.
// That means, that by pipelining, you've reduced congestion and allowed for your clock to be set to a higher speed, which then causes a speed up. So you have to balance latency and the speed-up that you get I guess. It's totally possible that the latency is cancelled out by the resulting speed-up, but that's probably different
// from case to case. The second awesome thing about pipelining is that way more of your CPU is being used at one time, since each cycle is doing all the stages of an instruction, just a different stage for each of it's currently loaded instructions. I'm not really sure why using more of the CPU is better though, TODO: research why that is better in this specific case.
// Another thing that causes parallelism is superscalar processors. Basically, they just have multiple instances of the pipeline and grab multiple instructions off the input stream and start processing them in parallel, relegating each to it's own pipe.
// There are probably tons of other things that a processor does, but even just those two (especially pipelining) are really cool concepts.
// Another important thing: because of the fact that a bunch of instructions are going down at the same time with pipelining and superscaling and all that, much more of the CPU logic is dedicated to edge cases such as program branches.
// For example, if a program branches, but you've already been simultaeniously executing the next few instructions after the branch, what do you do? You have to discard the effects of the instructions that you've just done, but for that to work, I assume you have to keep a sort of undo history or something.
// Basically, as one can see, it gets complicated incredibly quickly. Basically, the CPU is way more complicated now, it's like a factory the size of a small country packed onto a silicon chip.
// Because of all this parallelism and decision making (it's very much akin to a massively complicated program which has been translated to hardware), the CPU is potentially more accurately visualized as a stream of water that gets split up into many smaller streams that each go their own routes and maybe combine back up at the end,
// instead of a rigidly serial process.

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

HDC g;

void graphicsLoop() {			// TODO: Just expose this g stuff in the library so we don't have to do this boilerplate every time. Good idea or no?

	HDC finalG = GetDC(hWnd);
	g = CreateCompatibleDC(finalG);
	HBITMAP bmp = CreateCompatibleBitmap(finalG, windowWidth, windowHeight);
	SelectObject(g, bmp);

	HPEN particlePen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
	HBRUSH particleBrush = CreateSolidBrush(RGB(0, 255, 0));

	HPEN bgPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	HBRUSH bgBrush = CreateSolidBrush(RGB(0, 0, 0));

	std::vector<Particle> particles;
	for (int i = 0; i < 2; i++) {
		particles.push_back(Particle(Vector2f(rand() % windowWidth, rand() % windowHeight), Vector2f(0, 0), 20, 1));
	}
	Scene scene(particles);

	Renderer renderer(g);

	while (isAlive) {
		SelectObject(g, bgPen);
		SelectObject(g, bgBrush);
		Rectangle(g, 0, 0, windowWidth, windowHeight);
		SelectObject(g, particlePen);
		SelectObject(g, particleBrush);
		renderer.render(scene);
		BitBlt(finalG, 0, 0, windowWidth, windowHeight, g, 0, 0, SRCCOPY);
		//scene.step();
	}
}