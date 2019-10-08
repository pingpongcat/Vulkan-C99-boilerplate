#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

static VkInstance vulkan_instance = VK_NULL_HANDLE;
static VkSurfaceKHR vulkan_surface = VK_NULL_HANDLE;
static VkDevice vulkan_device = VK_NULL_HANDLE;
static VkSwapchainKHR vulkan_swapchain = VK_NULL_HANDLE;

int clamp(int32_t i, int32_t min, int32_t max);
static size_t fileGetLenght(FILE *file);

VkShaderModule VLK_CreateShaderModule(char *filename);

int main(void)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Vulkan_LoadLibrary(NULL);

	const char *vulkan_window_title = "Hello Vulkan";

	SDL_Window *vulkan_window 		= SDL_CreateWindow(vulkan_window_title,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			800,
			600,
			SDL_WINDOW_VULKAN); 

	uint32_t instance_extensions_count;
	SDL_Vulkan_GetInstanceExtensions(vulkan_window, &instance_extensions_count, NULL);
	const char* instance_extensions[instance_extensions_count];
	SDL_Vulkan_GetInstanceExtensions(vulkan_window, &instance_extensions_count, instance_extensions);

	static char const * instance_layers [] = {"VK_LAYER_LUNARG_standard_validation"};

	VkInstanceCreateInfo instance_create_info = {
		.sType 						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext 						= NULL,
		.flags 						= 0,
		.pApplicationInfo = &(VkApplicationInfo){
			.sType 					= VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext 					= NULL,
			.pApplicationName 		= vulkan_window_title,
			.applicationVersion 	= VK_MAKE_VERSION(0, 0, 1),
			.pEngineName 			= "No name",
			.engineVersion 			= VK_MAKE_VERSION(0, 0, 1),
			.apiVersion 			= VK_MAKE_VERSION(1, 0, 21),
		},
#ifdef NDEBUG
		.enabledLayerCount 			= 0,
#else
		.enabledLayerCount 			= sizeof(instance_layers) / sizeof(instance_layers[0]),
		.ppEnabledLayerNames 		= instance_layers,
#endif
		.enabledExtensionCount 		= instance_extensions_count,
		.ppEnabledExtensionNames 	= instance_extensions,
	};

	assert(vkCreateInstance(&instance_create_info, NULL, &vulkan_instance) == VK_SUCCESS);
	assert(SDL_Vulkan_CreateSurface(vulkan_window, vulkan_instance, &vulkan_surface) == 1);

	uint32_t device = 1; //assuming one device in the system
	VkPhysicalDevice physical_device;
	vkEnumeratePhysicalDevices(vulkan_instance, &device, &physical_device);

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

	VkQueueFamilyProperties queue_family_properties[queue_family_count];
	assert(queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &device, queue_family_properties);
	assert(queue_family_properties[0].queueFlags & VK_QUEUE_GRAPHICS_BIT); //first device queu

	VkBool32 surface_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, 0, vulkan_surface, &surface_support);
	assert(surface_support);

	char const * device_extensions [] = {"VK_KHR_swapchain"};

	VkDeviceCreateInfo device_create_info = {
		.sType 						= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext 						= NULL,
		.flags 						= 0,
		.queueCreateInfoCount 		= 1,
		.pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
			.sType 					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext 					= NULL,
			.flags 					= 0,
			.queueFamilyIndex 		= 0,
			.queueCount 			= 1,
			.pQueuePriorities 		= (const float []){1.0f},
		},
		.enabledLayerCount 			= 1,
		.ppEnabledLayerNames 		= instance_layers,
		.enabledExtensionCount 		= sizeof(device_extensions) / sizeof(device_extensions[0]),
		.ppEnabledExtensionNames 	= device_extensions,
		.pEnabledFeatures 			= NULL,
	};

	assert(vkCreateDevice(physical_device, &device_create_info, NULL, &vulkan_device) == VK_SUCCESS);

	uint32_t surface_formats_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vulkan_surface, &surface_formats_count, NULL);
	assert(surface_formats_count);

	VkSurfaceFormatKHR surface_formats[surface_formats_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vulkan_surface, &device, surface_formats);

	static VkFormat swapchain_image_format = VK_NULL_HANDLE;
	static VkColorSpaceKHR swapchain_color_space = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < surface_formats_count; ++i) {
		if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			swapchain_image_format = surface_formats[i].format;
			swapchain_color_space = surface_formats[i].colorSpace;
			break;
		}
	}

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, vulkan_surface, &surface_capabilities);

	int32_t vulkan_window_width, vulkan_window_height;
	SDL_Vulkan_GetDrawableSize(vulkan_window, &vulkan_window_width, &vulkan_window_height);
	vulkan_window_width = clamp(vulkan_window_width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
	vulkan_window_height = clamp(vulkan_window_height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

	VkSwapchainCreateInfoKHR swapchain_create_info = {
		.sType 						= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext 						= NULL,
		.flags 						= 0,
		.surface 					= vulkan_surface,
		.minImageCount 				= surface_capabilities.minImageCount,
		.imageFormat 				= swapchain_image_format,
		.imageColorSpace 			= swapchain_color_space,
		.imageExtent = {
			.width 					= vulkan_window_width,
			.height 				= vulkan_window_height
		},
		.imageArrayLayers 			= 1,
		.imageUsage 				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode 			= VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount 		= 0,
		.pQueueFamilyIndices 		= NULL,
		.preTransform 				= surface_capabilities.currentTransform,
		.compositeAlpha 			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode 				= VK_PRESENT_MODE_FIFO_KHR,
		.clipped 					= VK_TRUE,
		.oldSwapchain 				= NULL,
	};

	assert(vkCreateSwapchainKHR(vulkan_device, &swapchain_create_info, NULL, &vulkan_swapchain) == VK_SUCCESS);

	uint32_t swapchain_images_count = 0;
	vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &swapchain_images_count, NULL);
	assert(swapchain_images_count);
	VkImage swapchain_images[swapchain_images_count];
	vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &swapchain_images_count, swapchain_images);

	VkImageView swapchain_images_views[swapchain_images_count];

	for (uint32_t i = 0; i < swapchain_images_count; ++i) {

		VkImageViewCreateInfo image_view_create_info = {
			.sType 					= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext 					= NULL,
			.flags 					= 0,
			.image 					= swapchain_images[i],
			.viewType 				= VK_IMAGE_VIEW_TYPE_2D,
			.format 				= swapchain_image_format,
			.components = {
				.r 					= VK_COMPONENT_SWIZZLE_IDENTITY,
				.g 					= VK_COMPONENT_SWIZZLE_IDENTITY,
				.b 					= VK_COMPONENT_SWIZZLE_IDENTITY,
				.a 					= VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel 		= 0,
				.levelCount 		= 1,
				.baseArrayLayer 	= 0,
				.layerCount 		= 1
			}
		};

		assert(vkCreateImageView(vulkan_device, &image_view_create_info, NULL, &swapchain_images_views[i]) == VK_SUCCESS);
	}


	VkShaderModule vert_shader_module = VLK_CreateShaderModule("./vert.spv");
	VkShaderModule frag_shader_module = VLK_CreateShaderModule("./frag.spv");

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
		.sType 						= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage 						= VK_SHADER_STAGE_VERTEX_BIT,
		.module 					= vert_shader_module,
		.pName 						= "main"
	};

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
		.sType 						= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage 						= VK_SHADER_STAGE_FRAGMENT_BIT,
		.module 					= vert_shader_module,
		.pName 						= "main"
	};

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

	VkPipelineVertexInputStateCreateInfo vert_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount 				= 1,
		.pViewports = &(VkViewport) {
			.x 						= 0.0f,
			.y 						= 0.0f,
			.width 					= (float) vulkan_window_width,
			.height 				= (float) vulkan_window_height,
			.minDepth 				= 0.0f,
			.maxDepth 				= 1.0f
		},
		.scissorCount 				= 1,
		.pScissors = &(VkRect2D){
			.offset = {
				.x 					= 0,
				.y 					= 0
			},
			.extent = {
				.width 				= vulkan_window_width,
				.height 			= vulkan_window_height
			}
		}
	};

	VkPipelineRasterizationStateCreateInfo vulkan_rasterizer = {
		.sType 						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable 			= VK_FALSE,
		.polygonMode 				= VK_POLYGON_MODE_FILL,
		.lineWidth 					= 1.0f,
		.cullMode 					= VK_CULL_MODE_BACK_BIT,
		.frontFace 					= VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable 			= VK_FALSE,
		.depthBiasConstantFactor 	= 0.0f,
		.depthBiasClamp 			= 0.0f,
		.depthBiasSlopeFactor 		= 0.0f
	};

	VkPipelineMultisampleStateCreateInfo vulkan_multisampling = {
		.sType 						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable 		= VK_FALSE,
		.rasterizationSamples 		= VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading 			= 1.0f,
		.pSampleMask 				= NULL,
		.alphaToCoverageEnable 		= VK_FALSE,
		.alphaToOneEnable 			= VK_FALSE
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment  = {
		.colorWriteMask 			= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable 				= VK_FALSE,
		.srcColorBlendFactor 		= VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor 		= VK_BLEND_FACTOR_ZERO,
		.colorBlendOp 				= VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor 		= VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor 		= VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp 				= VK_BLEND_OP_ADD
	};


	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0, // Optional
		.pSetLayouts = NULL, // Optional
		.pushConstantRangeCount = 0, // Optional
		.pPushConstantRanges = NULL  // Optional
	};

	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	assert(vkCreatePipelineLayout(vulkan_device, &pipeline_layout_info, NULL, &pipeline_layout) == VK_SUCCESS);

	VkAttachmentDescription color_attachment = {
		.format = swapchain_image_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE
	};

	bool quit = false;
	SDL_Event event;
	while( !quit ){
		while( SDL_PollEvent( &event ) != 0 ){
			if( event.type == SDL_QUIT ){
				quit = true;
			}
		}
	}

	for (uint32_t i = 0; i < swapchain_images_count; ++i) {
		vkDestroyImageView(vulkan_device, swapchain_images_views[i], NULL);
	}

	vkDestroyPipelineLayout(vulkan_device, pipeline_layout, NULL);
	vkDestroyShaderModule(vulkan_device, vert_shader_module, NULL);
	vkDestroyShaderModule(vulkan_device, frag_shader_module, NULL);

	vkDestroySwapchainKHR(vulkan_device, vulkan_swapchain, NULL);
	vkDestroySurfaceKHR( vulkan_instance, vulkan_surface, NULL);
	vkDestroyDevice(vulkan_device, NULL);
	vkDestroyInstance(vulkan_instance, NULL);

	SDL_Quit();
	return 0;
}

static size_t fileGetLenght(FILE *file){

	size_t lenght;
	size_t currPos = ftell(file);
	fseek(file, 0 , SEEK_END);
	lenght = ftell(file);
	fseek(file, currPos, SEEK_SET);
	return lenght;
}

int clamp(int32_t i, int32_t min, int32_t max){
	if (i < min)
		return min;
	if (i > max)
		return max;
	return i;
}

VkShaderModule VLK_CreateShaderModule(char *filename){
	FILE *file = fopen(filename,"r");

	if (!file) {
		printf("Can't open file: %s\n", filename);
		return 0;
	}

	size_t lenght = fileGetLenght(file);

	const uint32_t *shader_descripteion = (const uint32_t *)calloc(lenght + 1, 1);
	if (!shader_descripteion) {
		printf("Out of memory when reading file: %s\n",filename);
		fclose(file);
		file = NULL;
		return 0;;
	}

	fread((void *)shader_descripteion,1,lenght,file);
	fclose(file);

	file = NULL;

	VkShaderModuleCreateInfo shader_module_create_info = {
		.sType 						= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize 					= lenght,
		.pCode 						= shader_descripteion
	};

	VkShaderModule shader_module;
	assert(vkCreateShaderModule(vulkan_device, &shader_module_create_info, NULL, &shader_module) == VK_SUCCESS); 
	free((void *)shader_descripteion);
	shader_descripteion = NULL;

	return shader_module;
}
