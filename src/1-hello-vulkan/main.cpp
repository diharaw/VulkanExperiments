#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

const char* kDeviceTypes[] = {
	"VK_PHYSICAL_DEVICE_TYPE_OTHER",
	"VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU",
	"VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU",
	"VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU",
	"VK_PHYSICAL_DEVICE_TYPE_CPU"
};

const char* kVendorNames[] = {
	"Unknown",
	"AMD",
	"IMAGINATION",
	"NVIDIA",
	"ARM",
	"QUALCOMM",
	"INTEL"
};

namespace VkVendorID
{
	enum 
	{
		AMD = 0x1002,
		IMAGINATION = 0x1010,
		NVIDIA = 0x10DE,
		ARM = 0x13B5,
		QUALCOMM = 0x5143,
		INTEL = 0x8086,
	};
}

const char* GetVendorName(uint32_t id)
{
	switch (id)
	{
	case 0x1002:
		return kVendorNames[1];
	case 0x1010:
		return kVendorNames[2];
	case 0x10DE:
		return kVendorNames[3];
	case 0x13B5:
		return kVendorNames[4];
	case 0x5143:
		return kVendorNames[5];
	case 0x8086:
		return kVendorNames[6];
	default:
		return kVendorNames[0];
	}
}

int main()
{
	VkInstance instance;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Test";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Experiment Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;

	instance_info.enabledExtensionCount = 0;
	instance_info.enabledLayerCount = 0;

	if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS)
	{
		std::cout << "Failed to created Vulkan instance" << std::endl;
	}

	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(device_count);

	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	std::cout << "Number of Physical Devices found: " << device_count << std::endl;

	VkPhysicalDevice physicalDevice;
	uint32_t vendorId = 0;

	for (auto& device : devices)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		vendorId = properties.vendorID;

		std::cout << std::endl;
		std::cout << "Vendor: " << GetVendorName(properties.vendorID) << std::endl;
		std::cout << "Name: " << properties.deviceName << std::endl;
		std::cout << "Type: " << kDeviceTypes[properties.deviceType] << std::endl;
		std::cout << "Driver: " << properties.driverVersion << std::endl;

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			physicalDevice = device;
	}

	uint32_t family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &family_count, nullptr);

	std::cout << std::endl;
	std::cout << "Number of Queue families: " << family_count << std::endl;

	std::vector<VkQueueFamilyProperties> families(family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &family_count, families.data());

	for (int i = 0; i < family_count; i++)
	{
		VkQueueFlags bits = families[i].queueFlags;

		std::cout << std::endl;
		std::cout << "Family " << i << std::endl;
		std::cout << "Supported Bits: " << "VK_QUEUE_GRAPHICS_BIT: " << ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "1" : "0") << ", " << "VK_QUEUE_COMPUTE_BIT: " << ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) ? "1" : "0") << ", " << "VK_QUEUE_TRANSFER_BIT: " << ((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) ? "1" : "0") << std::endl;
		std::cout << "Number of Queues: " << families[i].queueCount << std::endl;

		if (vendorId == VkVendorID::NVIDIA)
		{
			// Look for Transfer Queue
			if (!(bits & VK_QUEUE_GRAPHICS_BIT) && !(bits & VK_QUEUE_COMPUTE_BIT) && (bits & VK_QUEUE_TRANSFER_BIT))
				std::cout << "Found Transfer Queue Family" << std::endl;
			// Look for Async Compute Queue
			if (!(bits & VK_QUEUE_GRAPHICS_BIT) && (bits & VK_QUEUE_COMPUTE_BIT) && !(bits & VK_QUEUE_TRANSFER_BIT))
				std::cout << "Found Async Compute Queue Family" << std::endl;
		}
	}

	vkDestroyInstance(instance, nullptr);

	std::cin.get();

	return 0;
}