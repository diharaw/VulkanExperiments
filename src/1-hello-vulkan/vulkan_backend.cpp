#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdint.h>
#include <vector>

#include "vulkan_backend.h"
#include "const.h"

namespace vulkan_backend
{
	VkInstance g_instance { VK_NULL_HANDLE };

	// private methods

	void extension_info()
	{
		uint32_t count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

		std::cout << "Supported Instance Extenstions : " << std::endl;

		for (const auto& extension : extensions)
		{
			std::cout << extension.extensionName << std::endl;
		}
	}

	bool create_instance()
	{
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = WINDOW_TITLE;
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "Experiment Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		instance_info.enabledExtensionCount = glfwExtensionCount;
		instance_info.ppEnabledExtensionNames = glfwExtensions;
		instance_info.enabledLayerCount = 0;

		if (vkCreateInstance(&instance_info, nullptr, &g_instance) != VK_SUCCESS)
		{
			std::cout << "Failed to create vkInstance!" << std::endl;
			return false;
		}
		else
		{
			std::cout << "Successfully created vkInstance!" << std::endl;
			return true;
		}	
	}

	bool initialize()
	{
		if (!create_instance())
			return false;

		extension_info();

		return true;
	}

	void shutdown()
	{
		vkDestroyInstance(g_instance, nullptr);
	}
}