#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdint.h>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>

#include "vulkan_backend.h"
#include "const.h"

namespace vulkan_backend
{
	VkInstance				 g_instance		   { VK_NULL_HANDLE };
	VkPhysicalDevice		 g_physical_device { VK_NULL_HANDLE };
	VkDevice				 g_device;
	VkQueue					 g_graphics_queue;
	VkQueue					 g_present_queue;
	VkSurfaceKHR			 g_surface;
	VkSwapchainKHR			 g_swap_chain;
	VkFormat			     g_swap_chain_image_format;
	VkExtent2D				 g_swap_chain_extent;
	VkPipelineLayout		 g_pipeline_layout;
	VkRenderPass			 g_render_pass;
	VkPipeline				 g_graphics_pipeline;
	VkCommandPool			 g_command_pool;
	VkSemaphore				 g_image_availabel_sema;
	VkSemaphore				 g_render_finished_sema;

	VkDebugReportCallbackEXT g_debug_callback;

	std::vector<VkCommandBuffer> g_command_buffers;
	std::vector<VkImage> g_swap_chain_images;
	std::vector<VkImageView> g_swap_chain_image_views;
	std::vector<VkFramebuffer> g_swap_chain_framebuffers;

	GLFWwindow*				 g_window;
	
	const std::vector<const char*> g_validation_layers = 
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	// Device Extensions
	const std::vector<const char*> g_device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
	const bool g_enable_validation_layers = false;
#else
	const bool g_enable_validation_layers = true;
#endif

	struct QueueFamilyIndices
	{
		int graphics_family = -1;
		int present_family = -1;

		bool is_complete()
		{
			return graphics_family >= 0 && present_family >= 0;
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR		capabilities;
		std::vector<VkSurfaceFormatKHR> format;
		std::vector<VkPresentModeKHR>   present_modes;
	};

	// private methods

	static std::vector<char> read_file(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		
		if (!file.is_open())
			throw std::runtime_error("Failed to open file!");

		size_t size = (size_t)file.tellg();
		std::vector<char> buffer(size);

		file.seekg(0);
		file.read(buffer.data(), size);
		file.close();

		return buffer;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT obj_type,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layer_prefix,
		const char* msg,
		void* user_data)
	{
		std::cerr << "Validation Layer : " << msg << std::endl;

		return VK_FALSE;
	}

	VkResult create_debug_report_callback_ext(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) 
	{
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

		if (func != nullptr) 
			return func(instance, pCreateInfo, pAllocator, pCallback);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void destroy_debug_report_callback_ext(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

		if (func != nullptr)
			func(instance, callback, pAllocator);
	}

	std::vector<const char*> get_required_extensions()
	{
		std::vector<const char*> extensions;

		uint32_t ext_count = 0;
		const char** glfw_extensions;

		glfw_extensions = glfwGetRequiredInstanceExtensions(&ext_count);

		for (uint32_t i = 0; i < ext_count; i++)
			extensions.push_back(glfw_extensions[i]);

		if (g_enable_validation_layers)
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

		return extensions;
	}

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

	bool check_validation_layer_support() 
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : g_validation_layers) 
		{
			bool layer_found = false;

			for (const auto& layer_properties : available_layers) 
			{
				if (strcmp(layer_name, layer_properties.layerName) == 0) {
					layer_found = true;
					break;
				}
			}

			if (!layer_found) 
				return false;
		}

		return true;
	}

	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetWindowSize(g_window, &width, &height);

			VkExtent2D actual_extent = { width, height };
			actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
			return actual_extent;
		}
	}

	// Selects the best Swapping method (IMMEDIATE, FIFO, FIFO RELAXED, MAILBOX)
	VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
	{
		VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& available_present_mode : available_present_modes)
		{
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				best_mode = available_present_mode;
			else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				best_mode = available_present_mode;
		}

		return best_mode;
	}

	VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
	{
		if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& available_format : available_formats)
		{
			if (available_format.format == VK_FORMAT_B8G8R8A8_SNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return available_format;
		}

		return available_formats[0];
	}

	SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		// Get surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_surface, &details.capabilities);

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &present_mode_count, nullptr);

		if (present_mode_count != 0)
		{
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &present_mode_count, details.present_modes.data());
		}

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &format_count, nullptr);

		if (format_count != 0)
		{
			details.format.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &format_count, details.format.data());
		}

		return details;
	}

	QueueFamilyIndices find_queue_families(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);

		std::vector<VkQueueFamilyProperties> families(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

		for (int i = 0; i < family_count; i++)
		{
			if (families[i].queueCount > 0 && families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphics_family = i;

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_surface, &present_support);

			if (families[i].queueCount > 0 && present_support)
				indices.present_family = i;

			if (indices.is_complete())
				break;
		}

		return indices;
	}

	bool check_device_extension_support(VkPhysicalDevice device)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(g_device_extensions.begin(), g_device_extensions.end());

		for (const auto& extension : available_extensions)
		{
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	bool is_device_suitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			std::cout << "Selected Device : " << properties.deviceName << std::endl;

			QueueFamilyIndices indices = find_queue_families(device);

			bool extensions_supported = check_device_extension_support(device);
			bool swap_chain_adeqeute = false;

			if (extensions_supported)
			{
				SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
				swap_chain_adeqeute = !swap_chain_support.format.empty() && !swap_chain_support.present_modes.empty();
			}

			return indices.is_complete() && extensions_supported && swap_chain_adeqeute;
		}
		else
			return false;
	}

	void pick_physical_device()
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(g_instance, &device_count, nullptr);

		if (device_count == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(device_count);

		vkEnumeratePhysicalDevices(g_instance, &device_count, devices.data());

		for (const auto& device : devices)
		{
			if (is_device_suitable(device))
			{
				g_physical_device = device;
				break;
			}
		}

		if (g_physical_device == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU!");
	}

	void create_logical_device()
	{
		QueueFamilyIndices indices = find_queue_families(g_physical_device);

		std::vector<VkDeviceQueueCreateInfo> queue_infos;
		std::set<int> unique_queue_families = { indices.graphics_family, indices.present_family };

		float priority = 1.0f;
		for (int queue_family : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_info = {};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = queue_family;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &priority;
			queue_infos.push_back(queue_info);
		}

		VkPhysicalDeviceFeatures features = {};

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.pQueueCreateInfos = queue_infos.data();
		device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
		device_info.pEnabledFeatures = &features;
		device_info.enabledExtensionCount = static_cast<uint32_t>(g_device_extensions.size());
		device_info.ppEnabledExtensionNames = g_device_extensions.data();

		if (g_enable_validation_layers)
		{
			device_info.enabledLayerCount = static_cast<uint32_t>(g_validation_layers.size());
			device_info.ppEnabledLayerNames = g_validation_layers.data();
		}
		else
			device_info.enabledLayerCount = 0;

		if (vkCreateDevice(g_physical_device, &device_info, nullptr, &g_device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device!");

		vkGetDeviceQueue(g_device, indices.graphics_family, 0, &g_graphics_queue);
		vkGetDeviceQueue(g_device, indices.present_family, 0, &g_present_queue);
	}

	void create_swap_chain()
	{
		SwapChainSupportDetails swap_chain_support = query_swap_chain_support(g_physical_device);
		VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.format);
		VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
		VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities);

		uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

		if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
			image_count = swap_chain_support.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR create_info = {};

		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = g_surface;

		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		g_swap_chain_image_format = surface_format.format;
		g_swap_chain_extent = extent;

		QueueFamilyIndices indices = find_queue_families(g_physical_device);
		uint32_t queue_family_indices[] = { (uint32_t)indices.graphics_family, (uint32_t)indices.present_family };

		if (indices.graphics_family != indices.present_family)
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}

		create_info.preTransform = swap_chain_support.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(g_device, &create_info, nullptr, &g_swap_chain) != VK_SUCCESS)
			throw std::runtime_error("Failed to create swap chain!");

		uint32_t swap_image_count = 0;
		vkGetSwapchainImagesKHR(g_device, g_swap_chain, &swap_image_count, nullptr);
		g_swap_chain_images.resize(swap_image_count);
		vkGetSwapchainImagesKHR(g_device, g_swap_chain, &swap_image_count, g_swap_chain_images.data());
	}

	bool create_instance()
	{
		if (g_enable_validation_layers && !check_validation_layer_support())
			throw std::runtime_error("Validation layers requested, but not available!");

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

		
		auto extensions = get_required_extensions();

		instance_info.enabledExtensionCount = extensions.size();
		instance_info.ppEnabledExtensionNames = extensions.data();
		instance_info.enabledLayerCount = 0;

		if (g_enable_validation_layers) 
		{
			instance_info.enabledLayerCount = static_cast<uint32_t>(g_validation_layers.size());
			instance_info.ppEnabledLayerNames = g_validation_layers.data();
		}
		else
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

	void setup_debug_callback()
	{
		if (!g_enable_validation_layers)
			return;

		VkDebugReportCallbackCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		create_info.pfnCallback = debug_callback;

		if (create_debug_report_callback_ext(g_instance, &create_info, nullptr, &g_debug_callback) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug callback!");
	}

	void create_surface(GLFWwindow* window)
	{
		if (glfwCreateWindowSurface(g_instance, window, nullptr, &g_surface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");
	}

	void create_image_views()
	{
		g_swap_chain_image_views.resize(g_swap_chain_images.size());

		for (size_t i = 0; i < g_swap_chain_images.size(); i++)
		{
			VkImageViewCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = g_swap_chain_images[i];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = g_swap_chain_image_format;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(g_device, &create_info, nullptr, &g_swap_chain_image_views[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create image view!");
		}
	}

	VkShaderModule create_shader_module(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module;
		if (vkCreateShaderModule(g_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");

		return shader_module;
	}

	void create_render_pass()
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = g_swap_chain_image_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_att_ref = {};
		color_att_ref.attachment = 0;
		color_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_att_ref;

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};

		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		if (vkCreateRenderPass(g_device, &render_pass_info, nullptr, &g_render_pass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create render pass!");
	}

	void create_graphics_pipeline()
	{
		auto vert_bin = read_file("shaders/vert.spv");
		auto frag_bin = read_file("shaders/frag.spv");

		VkShaderModule vert_module = create_shader_module(vert_bin);
		VkShaderModule frag_module = create_shader_module(frag_bin);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_module;
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.pVertexBindingDescriptions = nullptr;
		vertex_input_info.vertexAttributeDescriptionCount = 0;
		vertex_input_info.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo input_assembly = {};

		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};

		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) g_swap_chain_extent.width;
		viewport.height = (float)g_swap_chain_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};

		scissor.offset = { 0, 0 };
		scissor.extent = g_swap_chain_extent;

		VkPipelineViewportStateCreateInfo viewport_state = {};

		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};

		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthBiasClamp = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling = {};

		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState color_blend_attachment = {};

		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending = {};

		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		VkDynamicState dynamic_states[]
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamic_state = {};

		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = 2;
		dynamic_state.pDynamicStates = dynamic_states;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};

		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pSetLayouts = nullptr;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = 0;

		if (vkCreatePipelineLayout(g_device, &pipeline_layout_info, nullptr, &g_pipeline_layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pDepthStencilState = nullptr;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = nullptr;
		pipeline_info.layout = g_pipeline_layout;
		pipeline_info.renderPass = g_render_pass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &g_graphics_pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline!");

		vkDestroyShaderModule(g_device, frag_module, nullptr);
		vkDestroyShaderModule(g_device, vert_module, nullptr);
	}

	void create_framebuffers()
	{
		g_swap_chain_framebuffers.resize(g_swap_chain_image_views.size());

		for (size_t i = 0; i < g_swap_chain_image_views.size(); i++)
		{
			VkImageView attachments[] = {
				g_swap_chain_image_views[i]
			};

			VkFramebufferCreateInfo framebuffer_info = {};

			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = g_render_pass;
			framebuffer_info.attachmentCount = 1;
			framebuffer_info.pAttachments = attachments;
			framebuffer_info.width = g_swap_chain_extent.width;
			framebuffer_info.height = g_swap_chain_extent.height;
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(g_device, &framebuffer_info, nullptr, &g_swap_chain_framebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer!");
		}
	}

	void create_command_pool()
	{
		QueueFamilyIndices indices = find_queue_families(g_physical_device);

		VkCommandPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = indices.graphics_family;
		pool_info.flags = 0;

		if (vkCreateCommandPool(g_device, &pool_info, nullptr, &g_command_pool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool");
	}

	void create_command_buffers()
	{
		g_command_buffers.resize(g_swap_chain_framebuffers.size());

		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = g_command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = (uint32_t)g_command_buffers.size();

		if (vkAllocateCommandBuffers(g_device, &alloc_info, g_command_buffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers");

		for (size_t i = 0; i < g_command_buffers.size(); i++)
		{
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			begin_info.pInheritanceInfo = nullptr;

			vkBeginCommandBuffer(g_command_buffers[i], &begin_info);

			VkRenderPassBeginInfo render_pass_info = {};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_info.renderPass = g_render_pass;
			render_pass_info.framebuffer = g_swap_chain_framebuffers[i];
			render_pass_info.renderArea.offset = { 0, 0 };
			render_pass_info.renderArea.extent = g_swap_chain_extent;

			VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
			render_pass_info.clearValueCount = 1;
			render_pass_info.pClearValues = &clear_color;

			vkCmdBeginRenderPass(g_command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(g_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, g_graphics_pipeline);
			vkCmdDraw(g_command_buffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(g_command_buffers[i]);

			if (vkEndCommandBuffer(g_command_buffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to record command buffer");
		}
	}

	void create_semaphores()
	{
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(g_device, &info, nullptr, &g_image_availabel_sema) != VK_SUCCESS ||
			vkCreateSemaphore(g_device, &info, nullptr, &g_render_finished_sema) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores");
		}
	}

	bool initialize(GLFWwindow* window)
	{
		g_window = window;

		if (!create_instance())
			return false;

		extension_info();
		setup_debug_callback();
		create_surface(window);
		pick_physical_device();
		create_logical_device();
		create_swap_chain();
		create_image_views();
		create_render_pass();
		create_graphics_pipeline();
		create_framebuffers();
		create_command_pool();
		create_command_buffers();
		create_semaphores();

		return true;
	}

	void draw()
	{
		vkQueueWaitIdle(g_present_queue);

		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(g_device, g_swap_chain, std::numeric_limits<uint64_t>::max(), g_image_availabel_sema, VK_NULL_HANDLE, &image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreate_swap_chain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image");
		}

		VkSubmitInfo submit_info = {};

		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_sema[] = { g_image_availabel_sema };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_sema;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &g_command_buffers[image_index];

		VkSemaphore signal_sema[] = { g_render_finished_sema };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_sema;

		if (vkQueueSubmit(g_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit command buffer");

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_sema;

		VkSwapchainKHR swapchains[] = { g_swap_chain };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		result = vkQueuePresentKHR(g_present_queue, &present_info);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			recreate_swap_chain();
			return;
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image");
		}
	}

	void cleanup_swap_chain()
	{
		for (size_t i = 0; i < g_swap_chain_framebuffers.size(); i++)
			vkDestroyFramebuffer(g_device, g_swap_chain_framebuffers[i], nullptr);

		vkFreeCommandBuffers(g_device, g_command_pool, static_cast<uint32_t>(g_command_buffers.size()), g_command_buffers.data());

		vkDestroyPipeline(g_device, g_graphics_pipeline, nullptr);
		vkDestroyPipelineLayout(g_device, g_pipeline_layout, nullptr);
		vkDestroyRenderPass(g_device, g_render_pass, nullptr);

		for (size_t i = 0; i < g_swap_chain_image_views.size(); i++)
			vkDestroyImageView(g_device, g_swap_chain_image_views[i], nullptr);

		vkDestroySwapchainKHR(g_device, g_swap_chain, nullptr);
	}

	void shutdown()
	{
		vkDeviceWaitIdle(g_device);

		cleanup_swap_chain();

		vkDestroySemaphore(g_device, g_render_finished_sema, nullptr);
		vkDestroySemaphore(g_device, g_image_availabel_sema, nullptr);

		vkDestroyCommandPool(g_device, g_command_pool, nullptr);

		destroy_debug_report_callback_ext(g_instance, g_debug_callback, nullptr);
		
		vkDestroyDevice(g_device, nullptr);
		vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
		vkDestroyInstance(g_instance, nullptr);
	}

	void recreate_swap_chain()
	{
		std::cout << "Recreating swap chain..." << std::endl;

		vkDeviceWaitIdle(g_device);

		cleanup_swap_chain();

		create_swap_chain();
		create_image_views();
		create_render_pass();
		create_graphics_pipeline();
		create_framebuffers();
		create_command_buffers();
	}
}