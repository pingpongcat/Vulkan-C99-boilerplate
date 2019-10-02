#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

int main(void)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Vulkan_LoadLibrary(NULL);
	// Vulkan is available, at least for compute
	uint32_t count;
	SDL_Window *vk_window = SDL_CreateWindow("Hello Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_VULKAN); 
	SDL_Vulkan_GetInstanceExtensions(vk_window, &count, NULL);
	const char* iex[count];
	SDL_Vulkan_GetInstanceExtensions(vk_window, &count, iex);
	static char const * ilay [] = {"VK_LAYER_LUNARG_standard_validation"};
	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &(VkApplicationInfo){
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = NULL,
			.pApplicationName = "test",
			.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
			.pEngineName = "test",
			.engineVersion = VK_MAKE_VERSION(0, 0, 1),
			.apiVersion = VK_MAKE_VERSION(1, 0, 21),
		},
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = ilay,
		.enabledExtensionCount = count,
		.ppEnabledExtensionNames = iex,
	};
	static VkInstance vk_instance = VK_NULL_HANDLE;

	assert(vkCreateInstance(&instance_create_info, NULL, &vk_instance) == VK_SUCCESS);


	static VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	assert(SDL_Vulkan_CreateSurface(vk_window, vk_instance, &vk_surface) == 1);

	float qp = 1;
	VkDeviceQueueCreateInfo device_queue_create = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = 0,
		.queueCount = 1,
		.pQueuePriorities = &qp,
	};

	uint32_t one = 1;
	VkPhysicalDevice pdev;
	vkEnumeratePhysicalDevices(vk_instance, &one, &pdev);

	VkQueueFamilyProperties qfam;
	uint32_t al;

	vkGetPhysicalDeviceQueueFamilyProperties(pdev, &al, NULL); //IF THIS LINE IS REMOVED, THERE IS NO SEGFAULT
	assert(al);

	vkGetPhysicalDeviceQueueFamilyProperties(pdev, &one, &qfam);
	assert(qfam.queueFlags & VK_QUEUE_GRAPHICS_BIT);
	VkBool32 sup_pres;

	vkGetPhysicalDeviceSurfaceSupportKHR(pdev, 0, vk_surface, &sup_pres);
	assert(sup_pres);

	vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, vk_surface, &al, NULL); //IF THIS LINE IS REMOVED, THERE IS NO SEGFAULT
	assert(al);

	VkSurfaceCapabilitiesKHR sc;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, vk_surface, &sc);

	VkSurfaceFormatKHR asd;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, vk_surface, &one, &asd);

	char const * dex [] = {"VK_KHR_swapchain"};
	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &device_queue_create,
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = ilay,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = dex,
		.pEnabledFeatures = NULL,
	};

	static VkDevice vk_device = VK_NULL_HANDLE;
	assert(vkCreateDevice(pdev, &device_create_info, NULL, &vk_device) == VK_SUCCESS);

	VkSurfaceCapabilitiesKHR scap;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, vk_surface, &scap);

	VkSwapchainCreateInfoKHR swap_chain_create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = vk_surface,
		.minImageCount = scap.minImageCount,
		.imageFormat = asd.format,
		.imageColorSpace = asd.colorSpace,
		.imageExtent.width = 800,
		.imageExtent.height = 600,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = scap.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = NULL,
	};
	
	static VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
	assert(vkCreateSwapchainKHR(vk_device, &swap_chain_create_info, NULL, &vk_swapchain) == VK_SUCCESS);

	SDL_Quit();
	return 0;
}
