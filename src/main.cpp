#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <random>
#include <memory>
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

std::mt19937 rng;

typedef unsigned long long guid;
guid random_guid() {
	return std::uniform_int_distribution<guid>()(rng);
}

struct Object {
	Object() {}
	Object(vec2 pos, vec2 speed) : pos(pos), speed(speed) {}
	long guid = random_guid();
	vec2 pos;
	vec2 speed;
};

struct World {
	list<Object> objects; // could be a vector, but reallocs will kill references in callbacks
	list<function<void()>> tick_events;
	list<function<void()>> queue;
	list<function<void(int, int, int, int)>> key_events;

	void tick(float dt) {
		for (auto& o : objects) {
			o.pos.x += o.speed.x * dt;
			o.pos.y += o.speed.y * dt;
		}
		for (auto t : tick_events) {
			t();
		}
	}

	Object* find_by_guid(guid g) {
		for (auto& o : objects) {
			if (o.guid == g) {
				return &o;
			}
		}
		return 0;
	}

	Object& add_object() {
		objects.push_back(Object());
		return objects.back();
	}

	void keyfun(int key, int scancode, int action, int mods) {
		/*
		 * [in]	window	The window that received the event.
		 * [in]	key	The keyboard key that was pressed or released.
		 * [in]	scancode	The system-specific scancode of the key.
		 * [in]	action	GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT.
		 * [in]	mods	Bit field describing which modifier keys were held down
		 */
		for (auto x : key_events) {
			x(key, scancode, action, mods);
		}
	}

	void bind_key(int key, std::function<void()> callback) {
		key_events.push_back([=](int key2, int scancode, int action, int mods){
			if (key2 == key and action == GLFW_PRESS) {
				callback();
			}
		});
	}

	void on_tick(std::function<void()> callback) {
		tick_events.push_back(callback);
	}
};

GLFWwindow* window;
World* cb_world;
void cb_key(GLFWwindow*, int key, int scancode, int action, int mods) {
	cb_world->keyfun(key, scancode, action, mods);
}



void WrapScreen(World& w, Object& o) {
	w.tick_events.push_back([&o](){
		float m = 0.1;
		float xa = -1-m, xb = 1+m, ya=-1-m, yb=1+m;
		float xw=xb-xa, yw=yb-ya;
		if (o.pos.x < xa) {
			o.pos.x += xw;
		}
		if (o.pos.x > xb) {
			o.pos.x -= xw;
		}
		if (o.pos.y < ya) {
			o.pos.y += yw;
		}
		if (o.pos.y > yb) {
			o.pos.y -= yw;
		}
	});
}

void Asteroid(World& w, Object& o) {
	o.pos.x = std::uniform_real_distribution<float>(-1, 1)(rng);
	o.pos.y = std::uniform_real_distribution<float>(-1, 1)(rng);
	float speed = 0.3;
	float angle = std::uniform_real_distribution<float>(0, 44./7)(rng);
	o.speed.x = cos(angle)*speed;
	o.speed.y = sin(angle)*speed;
	WrapScreen(w, o);
}


void Player(World& w, Object& o) {
	Asteroid(w, o);
	std::shared_ptr<float> angle(new float);

	w.on_tick([&o, angle](){
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			*angle += 0.1;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			*angle -= 0.1;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			float acc = 0.1;
			cout << *angle;
			o.speed.x += cos(*angle)*acc;
			o.speed.y += sin(*angle)*acc;
		}
	});

	w.bind_key(GLFW_KEY_SPACE, [&o](){
		float speed = 0.3;
		float angle = std::uniform_real_distribution<float>(0, 44./7)(rng);
		o.speed.x = cos(angle)*speed;
		o.speed.y = sin(angle)*speed;
	});
}

int main(void)
{
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_SAMPLES, 4);

	// window = glfwCreateWindow(1366, 768, "Hello World", glfwGetPrimaryMonitor(), 0);
	window = glfwCreateWindow(800, 480, "Hello World", 0, 0);

	if (!window)
	{
		cerr << "Could not create window" << endl;
		glfwTerminate();
		exit(-1);
	}

	World world;
	cb_world = &world;
	glfwSetKeyCallback(window, cb_key);

	for (int i=0; i<5; ++i) {
		Asteroid(world, world.add_object());
	}
	Player(world, world.add_object());


	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	double t0 = glfwGetTime();
	cout << glGetString(GL_VENDOR) << endl;
	cout << glGetString(GL_VERSION) << endl;
	cout << glGetString(GL_RENDERER) << endl;

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)glfwGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT) {
		wglSwapIntervalEXT(1);
	} else {
		cerr << "VSync not available" << endl;
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
		world.tick(dt);
		for (auto o : world.objects) {
			ding(o.pos.x, o.pos.y);
		}

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

	}

	glfwTerminate();
	return 0;
}


