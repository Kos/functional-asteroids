#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include "GL/gl.h"
#include "GL/wglext.h"
#include "GL/glu.h"
#include "foo.h"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::list;
using std::function;

#include "GLFW/glfw3.h"

void ding(float x, float y) {
	float a = 0.1;
	float b = 0.15;
	float p[] = {
		x+a, y,
		x, y+b,
		x-a, y,
		x, y-b
	};
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, p);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
}

struct vec2 {
	float x;
	float y;
};


std::list<std::function<void(int, int, int, int)>> on_key;
void keyfun(GLFWwindow * window, int key, int scancode, int action, int mods) {
	/*
	 * [in]	window	The window that received the event.
[in]	key	The keyboard key that was pressed or released.
[in]	scancode	The system-specific scancode of the key.
[in]	action	GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT.
[in]	mods	Bit field describing which modifier keys were held down
	 */
	for (auto x : on_key) {
		x(key, scancode, action, mods);
	}
}

void bind_key(int key, std::function<void()> callback) {
	on_key.push_back([=](int key2, int scancode, int action, int mods){
		if (key2 == key and action == GLFW_PRESS) {
			callback();
		}
	});
}


int SWAP_INTERVAL = 1;

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_SAMPLES, 4);

	/* Create a windowed mode window and its OpenGL context */

	window = glfwCreateWindow(1366, 768, "Hello World", glfwGetPrimaryMonitor(), NULL);

	if (!window)
	{
		cout << "Could not create window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwSetKeyCallback(window, keyfun);

	vector<vec2> objs = {
			{0, 0},
			{1, 1}
	};
	vector<vec2> speeds = {
			{0.2, 0},
			{0.2, 0.2}
	};

	bind_key(GLFW_KEY_SPACE, [&](){
		speeds[0].x *= -1;
	});

	list<function<void()>> queue;



	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	double t0 = glfwGetTime();
	cout << glGetString(GL_VENDOR) << endl;
	cout << glGetString(GL_VERSION) << endl;
	cout << glGetString(GL_RENDERER) << endl;

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)glfwGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT) {
		cerr << "VSync not available" << endl;
	} else {
		wglSwapIntervalEXT(1);
	}



	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		double t1 = glfwGetTime();
		float dt = (t1 - t0);
		t0 = t1;
		/* Render here */
		glClearColor(1,.98, 1 ,0);
		glClear(GL_COLOR_BUFFER_BIT);

		glColor3f(.5,.2,0);
		for (int i=0; i<objs.size(); ++i) {
			objs[i].x += speeds[i].x*dt;
			objs[i].y += speeds[i].y*dt;
			ding(objs[i].x, objs[i].y);
		}

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

