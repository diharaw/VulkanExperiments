#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define EXPERIMENT_DECLARE_MAIN(x)								\
int main()														\
{																\
	Application* app = new x();									\
	app->run();													\
	delete app;													\
	return 0;													\
}																\

class Application
{
public:
	Application();
	virtual ~Application();
	void run();

private:
	bool init_internal();
	void shutdown_internal();
	void update();

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

	virtual bool init() = 0;
	virtual void render() = 0;
	virtual void shutdown() = 0;

public:
	static bool keys[1024];
	static bool	mouse[5];

private:
	GLFWwindow* _window;
};
