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

// SIDE-NOTE THAT I'M WRITING DOWN TO REMEMBER IT IN FUTURE, HAS VERY LITTLE TO DO WITH THIS CODEBASE:
// ELF sections (or segments, I'm not sure, although someone online said that the two words have slightly different meanings and cannot be used interchangeably, so you still have to figure that out (TODO)):
// .text --> code is stored here (executable, obviously, but also non-writeable for security reasons (you can use a few syscalls to change the permissions of the different sections though and make the text portion writeable (useful for self-modifying code))).
// .data --> storage for global and/or static variables, because those have a default value that the OS can just write into your virtual memory before your program starts (default value specified by setting them equal to something in C++ (int something = 1 in global scope for example)).
//						data section has read/write access since the variables can be changed at runtime. AFAIK, it's up to the compiler if it wants to put your values in the data section, it could also just lazy initialize them when they are used for the first time, but that's more expensive for the application and a really dumb
//						thing to do so I don't know why the compiler would do that. It's different for static variables inside functions though, because those could depend on values that are not known at compile-time, eliminating the possibility of using the data section to initialize. Here, lazy initialization is garanteed by the standard.
// .bss -->	AFAIK, this one isn't an actual section full of data in the ELF file, it's probably just a span that specifies from which address to which other address zeros should be written, this is done as an optimization so that global/static variables that are 0 don't have to be unnecessarily written to data section and don't have to
//						make the ELF bigger. The OS zeros everything specified by the .bss section out and then starts your program.
// .rodata --> This is read-only data, used for string literal storage and (I assume) for global const variables. It's sometimes combined with .text as an optimization so that the OS has less to worry about, but that's not super important.
//						If you want string literals to be in the .data section instead of the .rodata section, because you want to change them at runtime for example, you can do char[] = "abc", which is the same thing as char[] = { 'a', 'b', 'c', '\0' }. This language feature was put in to (I assume) make it easier to initialize char arrays.
//						I'm not totally sure that the string literal won't also exist in the .rodata section, but why would it, the compiler wouldn't be so stupid as to do such unnecessary stuff, so the char[] method is probably exactly what you would be looking for.
// .tdata and .tbss --> thread local versions of bss and data, which are (I assume) loaded once on program startup, so you have access to the variables, but again everytime you create a new thread (so that the thread has access to it's own set of the variables, each initialized to the default value specified in .tdata).
// What is thread local? Thread local variables are basically like global variables, but they don't get carried across threads (threads have different stacks, but everything else, including .data segment, where the global variables reside, is shared between them (obviously tdata and tbss are not shared, thats the whole point)).
//				So global variables can be changed from all currently active threads of a process, but thread local variables (you can create them with the thread_local attribute on global variables), are copied for each thread so that each thread has it's own copy of the variables.
//				The copying is done by the runtime, but I'm very much not an expert on that, still confused about how exactly that gets accomplished.
// ANOTHER SUPER IMPORTANT THING: When a new thread is created, like I've already said, it gets a new stack, but it has the same virtual address space as the other threads, which is due to the requirement of needing to be able to share pointers and access the same data (think global vars stored in .data (and other data including each other's .tbss and .tdata data and each other's stacks, which is really cool)).
//		The problem with that is that the stacks are in the same address space, so they create limits for each other (the stack is already limited by the maximum addressable size of virtual memory, which is determined by the bitwidth of the machine (32 or 64-bit machines), and also by the memory layout in virtual memory.
//		it definitely can't expand forever, take a look at a typical memory layout for a process and then you'll understand better. I think a stack can have a max size of 80MB in most systems or something, but thats the 32-bit version so idk how it is on 64-bit machines.)
//		Creating threads adds more stacks, which (at least on 32-bit systems), presumably causes some memory issues. It seems probable that the maximum stack size goes down when you allocate new thread stack storage and such, which is super interesting, but shouldn't ever be a problem since heavy duty allocations should be
//		done in the heap anyway, which is shared between threads so I guess congestion becomes less of a problem, although if you need to allocate a bunch of copies of the same huge array for each thread in the heap, obviously you can still encounter the edge of virtual memory or whatever other bounds you've been given by your
//		memory layout.
// ANOTHER INTERESTING THING: Once a CPU gets put into virtual addressing mode, it applies for all processes, even the kernel, which means that, when you do a syscall and the system needs to switch into the kernels context after doing the interrupt (although interrupts are not the only way to implement syscalls), the virtual addresses
// need to be remapped so the kernel can use it's virtual addresses to address the appropriate physical memory. This is apparently a rather expensive operation, which is why it is avoided as much as possible (in the context of switching between regular programs that is). To avoid this when switching to kernel memory, which presumably
// happens way more often than regular task switches because everyones probably doing syscalls all the time, kernel memory's virtual address space is set to be present in every programs address space and is just made inaccessible to every program. In a 3/1 distribution, a programs virtual address space is divided so that
// the program has access to 3/4 of it and the kernel has access to the last 1/4, which in the context of 32-bit systems means that the program has 3 GB and the kernel has 1GB. For 64-bit systems the division is often half-half, but I'm haven't researched that all that much. That allows the kernel space of virtual memory to be
// constantly mapped to the kernels physical memory, and when your doing a syscall or something, no expensive virtual memory switching is required, the context is just switched from user to kernel mode and then the machine just starts doing things with the kernel memory in your virtual memory.
// The virtual kernel memory is also mmapped so that everyones kernel memory sections stay synchronized I believe. This isn't super hard because by mapping the virtual memory to the same physical memory, you've got this behaviour for free, which is how mmap works.
// This is done so that the kernel can maintain it's state regardless of which processes virtual memory it is currently residing in, at least that would make sense to me.

// ANOTHER THING: I've mentioned this a couple times, but I'll do it again, it's possible to create static variables in functions. I didn't know this before, and I think it's a weird feature for C++ to have, since I don't really know when I would ever use it, but it's possible.
// The behaviour is such that the variable gets initialized the first time the initialization code is processed in the function. One main difference, is that the lifetime of the variable is the lifetime of the program. It doesn't get destroyed when the function returns, it gets destroyed when the program exits.
// The second main difference is that you can call the function twice but the variable only gets initialized once. Static function variables are lazy initialized, so the first initialization is when the static variable gets initialized, and every time you call the function again after that and the initialization code is passed,
// it does nothing. AFAIK, it doesn't even update the value in case the values that are used to initialize have changed since the first initialization, it just straight up leaves it alone for the rest of the program. It's not const though, so you can still change it as you please manually.
// Since this all heavily depends on the runtime behaviour of your program and what branches the program takes and when the function with the static variable gets called, the compiler has to put in a little bit of code that makes sure that you never initialize the static variable twice. This is probably done through a simple
// boolean, but it is still technically an inefficiency. By moving the static variable outside the scope of the function and making it global, you avoid this extra runtime penalty (only as long as C++ doesn't do lazy initialization on global variables, I don't think so because I cant think of a situation where C++ would let you
// initialize a global variable with something that isn't known at compile-time, if it does, then lazy initialization is still an issue and if you don't want it, you have to declare the variable at global scope and then initialize it somewhere specific (default constructor might get in your way there), TODO: make this thing more precise by looking this stuff up and removing some of your guess-work from this comment). If you still want to
// initialize the variable with something that isn't known at compile-time, which is possible with static variables but (probably) not with global variables, then you have to just create the global variable, but then use a move assignment operator with a temporary object constructed with some sort of constructor to set the global variable equal
// to whatever value you want it be somewhere in your code. You've basically taken the lazy initialization out of the runtimes hands and hardcoded a spot where it's done, which avoids the overhead of normal lazy initialization.

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

int mouseX;
int mouseY;
LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_MOUSEMOVE:
		mouseX = GET_X_LPARAM(lParam);
		mouseY = GET_Y_LPARAM(lParam);
		return 0;
	}
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
	for (int i = 0; i < 100; i++) {
		particles.push_back(Particle(Vector2f((rand() % (windowWidth - 200)) + 100, (rand() % (windowHeight - 200)) + 100), Vector2f(0.1f, 0), 7, 1));
	}
	//particles.push_back(Particle(Vector2f(100, 100), Vector2f(0.1f, 0), 20, 1));
	//particles.push_back(Particle(Vector2f(200, 100), Vector2f(-0.1f, 0), 20, 1));
	//particles.push_back(Particle(Vector2f(150, 150), Vector2f(0, -0.1f), 20, 1));
	Scene scene;
	scene.loadParticles(particles);
	scene.loadSize(windowWidth, windowHeight);
	scene.postLoadInit();

	Renderer renderer(g);

	mouseX = windowWidth / 2;
	mouseY = windowHeight / 2;

	while (isAlive) {
		SelectObject(g, bgPen);
		SelectObject(g, bgBrush);
		Rectangle(g, 0, 0, windowWidth, windowHeight);
		SelectObject(g, particlePen);
		SelectObject(g, particleBrush);
		renderer.render(scene);
		BitBlt(finalG, 0, 0, windowWidth, windowHeight, g, 0, 0, SRCCOPY);
		scene.step();
		for (int i = 0; i < scene.particleCount; i++) {
			scene.particles[i].vel += (Vector2f(mouseX, mouseY) - scene.particles[i].pos).normalize() * 0.01f;
			scene.particles[i].vel *= 0.995f;
		}
	}
}