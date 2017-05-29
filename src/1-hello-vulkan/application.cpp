#include "application.h"
#include "const.h"
#include "vulkan_backend.h"

bool Application::keys[1024];
bool Application::mouse[5];

Application::Application()
{

}

Application::~Application()
{

}

bool Application::init_internal()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);

	glfwSetKeyCallback(_window, key_callback);
	glfwSetCursorPosCallback(_window, mouse_callback);
	glfwSetScrollCallback(_window, scroll_callback);

	if (!_window)
		return false;

	return vulkan_backend::initialize();
}

void Application::shutdown_internal()
{
	vulkan_backend::shutdown();
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Application::update()
{
	glfwPollEvents();
}

void Application::run()
{
	if (!init_internal())
		return;

	if (!init())
		return;

	while (!glfwWindowShouldClose(_window))
	{
		update();
		render();
	}

	shutdown();
	shutdown_internal();
}

void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

void Application::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{

}
