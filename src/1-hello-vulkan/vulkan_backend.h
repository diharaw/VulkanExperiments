#pragma once

struct GLFWwindow;

namespace vulkan_backend
{
	extern bool initialize(GLFWwindow* window);
	extern void draw();
	extern void shutdown();
	extern void recreate_swap_chain();
}
