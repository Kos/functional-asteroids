#include <iostream>
#include <array>
#include <vector>
#include <list>
#include <functional>
#include <algorithm>
#include <random>
#include <memory>
#include "GL/gl.h"
#include "GL/wglext.h"
#include "GL/glu.h"
#include "GLFW/glfw3.h"
#include "countof.h"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::array;
using std::list;
using std::function;


struct vec2 {
	float x;
	float y;

	float dist2(vec2 b) {
		float dx = b.x-x, dy = b.y-y, len2=dx*dx+dy*dy;
		return len2;
	}
};

std::mt19937 rng;

typedef unsigned long long guid;
guid random_guid() {
	return std::uniform_int_distribution<guid>()(rng);
}

struct Object {
	long guid = random_guid();
	vec2 pos;
	vec2 speed;
	float angle = 0;
	float angular = 0;

	bool operator==(const Object& other) {
		return guid == other.guid;
	}
};




template<typename T, typename... Args>
struct OwnedCallback {
	Object& o;
	function<T(Args...)> cb;

	T operator()(Args... args) {
		return cb(args...);
	}
};

struct Collision {
	// For now, simple quadratic collision
	struct Entry {
		Object& o;
		float size;
	};
	list<Entry> chaff;
	list<Entry> bullets;

	void check(function<void(Object&, Object&)> fn) {
		for (auto x : chaff) {
			for (auto y : bullets) {
				float overlap = x.size+y.size;
				if (x.o.pos.dist2(y.o.pos) < overlap*overlap) fn(x.o, y.o);
			}
		}
		for (auto x : chaff) {
			for (auto y : chaff) {
				if (x.o == y.o) continue;
				float overlap = x.size+y.size;
				if (x.o.pos.dist2(y.o.pos) < overlap*overlap) fn(x.o, y.o);
			}
		}
	}
};

const float AST[] {
	0.4, 0,
	0, 0.4,
	-0.4, 0,
	0, -0.4,
};
const float SDX =.4, SYG=.8, SYD=.2, SYDD=.4;
const float SHIP[] {
	-SDX, -SYDD,
	0, SYG,
	SDX, -SYDD,
	0, -SYD,
};
const float BULLET[] {
	0, -.3,
	0, .3,
};

const float* const MODELS[] { AST, SHIP, BULLET };
static constexpr int SIZES[] { 4, 4, 2 };


struct Renderer {

	struct Entry {
		Object& o;
		unsigned model;
	};
	list<Entry> entries;

	void add(Object& o, unsigned model) {
		entries.push_back(Entry { o, model });
	}

	void render() {
		glEnableClientState(GL_VERTEX_ARRAY);
		for (auto& e: entries) {
			render(e);
		}
	}

	void render(Entry& e) {
		float x = e.o.pos.x;
		float y = e.o.pos.y;
		const float* p = MODELS[e.model];
		int c = SIZES[e.model];
		glPushMatrix();
		glTranslatef(x, y, 0);
		if (e.o.angle) {
			float angle = (e.o.angle) - M_PI/2;
			glRotatef(angle*180/M_PI, 0, 0, 1);
		}
		glVertexPointer(2, GL_FLOAT, 0, p);
		glDrawArrays(GL_LINE_LOOP, 0, c);
		glPopMatrix();
	}
};


struct Messages {

	struct Entry {
		Object& listener;
		const char* msg;
		function<void(Object&)> cb;
	};
	list<Entry> entries;

	void listen(Object& o, const char* msg, function<void(Object&)> cb) {
		entries.push_back(Entry{o, msg, cb});
	}

	void send(Object& self, Object& tgt, const char* msg) {
		auto i = std::find_if(entries.begin(), entries.end(), [&tgt, msg](Entry& e){ return e.listener == tgt && 0 == strcmp(e.msg, msg); });
		if (i != entries.end()) {
			i->cb(self);
		}
	}
};

struct World {
	list<Object> objects; // could be a vector, but reallocs will kill references in callbacks
	list<OwnedCallback<void>> tick_events;
	list<OwnedCallback<void>> queue;
	list<OwnedCallback<void, int, int, int, int>> key_events;
	list<Object*> killQueue;

	Collision collisions;
	Renderer renderer;
	Messages messages;

	void tick(float dt) {
		for (auto& o : objects) {
			o.pos.x += o.speed.x * dt;
			o.pos.y += o.speed.y * dt;
			o.angle += o.angular * dt;
		}
		for (auto t : tick_events) {
			t();
		}
		collisions.check([this](Object& a, Object& b) {
			cout << "collision: " << a.guid << " x " << b.guid << endl;
			messages.send(a, b, "collide");
			messages.send(b, a, "collide");
		});
		for (auto o : killQueue) {
			real_kill(*o);
		}
		killQueue.erase(killQueue.begin(), killQueue.end());
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

	void bind_key(int key, Object& o, std::function<void()> callback) {
		key_events.push_back(OwnedCallback<void, int, int, int, int>{o, [=](int key2, int scancode, int action, int mods){
			if (key2 == key and action == GLFW_PRESS) {
				callback();
			}
		}});
	}

	void on_tick(Object& o, std::function<void()> callback) {
		tick_events.push_back(OwnedCallback<void>{o, callback});
	}

	void kill(Object& o) {
		killQueue.push_back(&o);
	}

	void real_kill(Object& o) {
		cout << "Killing: " << o.guid << endl;
		key_events.remove_if([&](OwnedCallback<void, int, int, int, int>& oc) { return oc.o == o; });
		tick_events.remove_if([&](OwnedCallback<void>& oc) { return oc.o == o; });
		queue.remove_if([&](OwnedCallback<void>& oc) { return oc.o == o; });
		collisions.chaff.remove_if([&](Collision::Entry& e) { return e.o == o; });
		collisions.bullets.remove_if([&](Collision::Entry& e) { return e.o == o; });
		renderer.entries.remove_if([&](Renderer::Entry& e) { return e.o == o; });
		objects.remove(o);

	}
};

GLFWwindow* window;
World* cb_world;
void cb_key(GLFWwindow*, int key, int scancode, int action, int mods) {
	cb_world->keyfun(key, scancode, action, mods);
}



void WrapScreen(World& w, Object& o) {
	w.tick_events.push_back(OwnedCallback<void>{o, [&o](){
		float m = 0.1;
		float xa = -16-m, xb = 16+m, ya=-9-m, yb=9+m;
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
	}});
}

void KillWhenExitingScreen(World& w, Object& o) {
	w.tick_events.push_back(OwnedCallback<void>{o, [&w, &o]() { // This can run less often
		float xa=-16, xb=16, ya=-9, yb=9;
		if (o.pos.x < xa or o.pos.x > xb or o.pos.y < ya or o.pos.y > yb) {
			w.kill(o);
		}
	}});
}

void Asteroid(World& w, Object& o) {
	w.renderer.add(o, 0);
	o.angular = 0.1;
	o.pos.x = std::uniform_real_distribution<float>(-16, 16)(rng);
	o.pos.y = std::uniform_real_distribution<float>(-9, 9)(rng);
	float speed = 0.3;
	float size = .4;
	float angle = std::uniform_real_distribution<float>(0, 44./7)(rng);
	o.speed.x = cos(angle)*speed;
	o.speed.y = sin(angle)*speed;
	WrapScreen(w, o);
	w.collisions.chaff.push_back(Collision::Entry{o, size});
	w.messages.listen(o, "damage", [&](Object&) {
		cout << "Received damage, killing";
		w.kill(o);
	});
}


void Bullet(World& w, Object& o) {
	KillWhenExitingScreen(w, o);
	w.renderer.add(o, 2);
	float size = 0.2;
	w.collisions.bullets.push_back(Collision::Entry{o, size});
	w.messages.listen(o, "collide", [&](Object& hit) {
		cout << "Received collide, sending damage" << endl;
		w.messages.send(o, hit, "damage");
	});
}


void Player(World& w, Object& o) {
	float size = .5;
	o.pos.x = o.pos.y = 0;
	o.speed.x = o.speed.y = 0;
	WrapScreen(w, o);
	w.collisions.chaff.push_back(Collision::Entry{o, size});
	w.renderer.add(o, 1);

	w.on_tick(o, [&o](){
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			o.angle -= 0.1;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			o.angle += 0.1;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			float acc = 0.1;
			o.speed.x += cos(o.angle)*acc;
			o.speed.y += sin(o.angle)*acc;
		}
	});
	w.bind_key(GLFW_KEY_SPACE, o, [&w, &o](){
		auto& as = w.add_object();
		Bullet(w, as);
		as.pos = o.pos;
		as.angle = o.angle;
		float v = 16;
		as.speed.x = cos(o.angle)*v;
		as.speed.y = sin(o.angle)*v;
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

	glMatrixMode(GL_PROJECTION);
	glOrtho(-16, 16, -9, 9, 1, -1);
	glMatrixMode(GL_MODELVIEW);

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
		world.renderer.render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
