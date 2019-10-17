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

const int MAX_FRAMES_IN_FLIGHT = 2;

int main(void)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Vulkan_LoadLibrary(NULL);

	const char *vulkan_window_title = "Hello Vulkan";

	SDL_Window *vulkan_window = SDL_CreateWindow(vulkan_window_title,
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
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &(VkApplicationInfo){
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = NULL,
			.pApplicationName = vulkan_window_title,
			.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
			.pEngineName = "No name",
			.engineVersion = VK_MAKE_VERSION(0, 0, 1),
			.apiVersion = VK_MAKE_VERSION(1, 0, 21),
		},
#ifdef NDEBUG
		.enabledLayerCount = 0,
#else
		.enabledLayerCount = sizeof(instance_layers) / sizeof(instance_layers[0]),
		.ppEnabledLayerNames = instance_layers,
#endif
		.enabledExtensionCount = instance_extensions_count,
		.ppEnabledExtensionNames = instance_extensions,
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
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.queueFamilyIndex = 0,
			.queueCount	= 1,
			.pQueuePriorities = (const float []){1.0f},
		},
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = instance_layers,
		.enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]),
		.ppEnabledExtensionNames = device_extensions,
		.pEnabledFeatures = NULL,
	};

	assert(vkCreateDevice(physical_device, &device_create_info, NULL, &vulkan_device) == VK_SUCCESS);
	
	VkQueue device_queue = VK_NULL_HANDLE;
	vkGetDeviceQueue(vulkan_device,0, 0, &device_queue); //pick que family of index 0

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
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = vulkan_surface,
		.minImageCount = surface_capabilities.minImageCount,
		.imageFormat = swapchain_image_format,
		.imageColorSpace = swapchain_color_space,
		.imageExtent = {
			.width = vulkan_window_width,
			.height = vulkan_window_height
		},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = NULL,
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
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.image = swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchain_image_format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		assert(vkCreateImageView(vulkan_device, &image_view_create_info, NULL, &swapchain_images_views[i]) == VK_SUCCESS);
	}

	VkShaderModule vert_shader_module = VLK_CreateShaderModule("./vert.spv");
	VkShaderModule frag_shader_module = VLK_CreateShaderModule("./frag.spv");

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &(VkAttachmentDescription){
				.format = swapchain_image_format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		.subpassCount = 1,
		.pSubpasses = &(VkSubpassDescription) {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &(VkAttachmentReference){
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}
		},
		.dependencyCount = 1,
		.pDependencies = &(VkSubpassDependency){
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		}

	};

	VkRenderPass render_pass = VK_NULL_HANDLE;
	assert(vkCreateRenderPass(vulkan_device, &render_pass_info, NULL, &render_pass) == VK_SUCCESS);

	VkFramebuffer swapchain_frame_buffers[swapchain_images_count];

	for (uint32_t i = 0; i < swapchain_images_count; ++i) {

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = render_pass,
			.attachmentCount = 1,
			.pAttachments = &swapchain_images_views[i],
			.width = vulkan_window_width,
			.height = vulkan_window_height,
			.layers = 1
		};
		assert(vkCreateFramebuffer(vulkan_device, &framebuffer_info, NULL, &swapchain_frame_buffers[i]) == VK_SUCCESS);
	}

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0, // Optional
		.pSetLayouts = NULL, // Optional
		.pushConstantRangeCount = 0, // Optional
		.pPushConstantRanges = NULL  // Optional
	};

	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	assert(vkCreatePipelineLayout(vulkan_device, &pipeline_layout_info, NULL, &pipeline_layout) == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shader_stages,
		.pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = NULL,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = NULL
		},
		.pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		},
		.pViewportState = &(VkPipelineViewportStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &(VkViewport) {
				.x = 0.0f,
				.y = 0.0f,
				.width = (float) vulkan_window_width,
				.height = (float) vulkan_window_height,
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			},
			.scissorCount = 1,
			.pScissors = &(VkRect2D){
				.offset = {
					.x = 0,
					.y = 0
				},
				.extent = {
					.width = vulkan_window_width,
					.height = vulkan_window_height
				}
			}
		},
		.pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.lineWidth = 1.0f,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f
		},
		.pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.sampleShadingEnable = VK_FALSE,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.minSampleShading = 1.0f,
			.pSampleMask = NULL,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		},
		.pDepthStencilState = NULL, // Optional
		.pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY, // Optional
			.attachmentCount = 1,
			.pAttachments = &(VkPipelineColorBlendAttachmentState) {
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				.blendEnable = VK_FALSE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD
			},
			.blendConstants[0] = 0.0f, // Optional
			.blendConstants[1] = 0.0f, // Optional
			.blendConstants[2] = 0.0f, // Optional
			.blendConstants[3] = 0.0f, // Optional
		},
		.pDynamicState = NULL, // Optional
		.layout = pipeline_layout,
		.renderPass = render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
		.basePipelineIndex = -1 // Optional
	};

	VkPipeline graphics_pipeline = VK_NULL_HANDLE;
	assert(vkCreateGraphicsPipelines(vulkan_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &graphics_pipeline) == VK_SUCCESS);


	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = 0, //assuming one graphics queue family
		.flags = 0 // Optional
	};

	VkCommandPool command_pool = VK_NULL_HANDLE;
	assert(vkCreateCommandPool(vulkan_device, &pool_info, NULL, &command_pool) == VK_SUCCESS);

	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = swapchain_images_count
	};

	VkCommandBuffer command_buffers[swapchain_images_count];
	assert(vkAllocateCommandBuffers(vulkan_device, &alloc_info, command_buffers) == VK_SUCCESS);

	for (size_t i = 0; i < swapchain_images_count; i++) {

		VkCommandBufferBeginInfo begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0, // Optional
			.pInheritanceInfo = NULL // Optional
		};
		assert(vkBeginCommandBuffer(command_buffers[i], &begin_info) == VK_SUCCESS);
	}

	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

	for (uint32_t i = 0; i < swapchain_images_count; ++i) {

		VkRenderPassBeginInfo render_pass_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = render_pass,
			.framebuffer = swapchain_frame_buffers[i],
			.renderArea.offset.x = 0.0f,
			.renderArea.offset.y = 0.0f,
			.renderArea.extent.width = vulkan_window_width,
			.renderArea.extent.height = vulkan_window_height,
			.clearValueCount = 1,
			.pClearValues = &clear_color

		};

		vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
		vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(command_buffers[i]);

		assert(vkEndCommandBuffer(command_buffers[i]) == VK_SUCCESS);
	}

	VkSemaphoreCreateInfo semaphore_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

	VkSemaphore image_available_semaphore[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphore[MAX_FRAMES_IN_FLIGHT];
	
	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	VkFence in_flight_fence[MAX_FRAMES_IN_FLIGHT];

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		assert(vkCreateSemaphore(vulkan_device, &semaphore_info, NULL, &image_available_semaphore[i]) == VK_SUCCESS);
		assert(vkCreateSemaphore(vulkan_device, &semaphore_info, NULL, &render_finished_semaphore[i]) == VK_SUCCESS);
		assert(vkCreateFence(vulkan_device, &fence_info, NULL, &in_flight_fence[i]) == VK_SUCCESS);
	}

	size_t current_frame = 0;
	bool quit = false;

	SDL_Event event;
	while( !quit ){
		while( SDL_PollEvent( &event ) != 0 ){
			switch (event.type) {
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							quit = true;
							break;
						default:
							break;
					}
					break;
				default:
					break;
					
			}
		}
		//Body
		vkWaitForFences(vulkan_device, 1, &in_flight_fence[current_frame], VK_TRUE, UINT64_MAX);
		vkResetFences(vulkan_device, 1, &in_flight_fence[current_frame]);

		uint32_t image_index;
		vkAcquireNextImageKHR(vulkan_device, vulkan_swapchain, UINT64_MAX, image_available_semaphore[current_frame], VK_NULL_HANDLE, &image_index);

		VkSemaphore wait_semaphores[] = {image_available_semaphore[current_frame]};
		VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSemaphore signal_semaphores[] = {render_finished_semaphore[current_frame]};

		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = wait_semaphores,
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &command_buffers[image_index],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signal_semaphores
		};

		assert(vkQueueSubmit(device_queue, 1, &submit_info, in_flight_fence[current_frame]) == VK_SUCCESS);

		VkSwapchainKHR swapchains[] = {vulkan_swapchain};

		VkPresentInfoKHR present_info = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signal_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &image_index,
			.pResults = NULL // Optional
		};

		//assuming present ation and graphics queues are the same one
		vkQueuePresentKHR(device_queue, &present_info);
		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	vkDeviceWaitIdle(vulkan_device);

	//Cleaning
	for (uint32_t i = 0; i < swapchain_images_count; ++i) {
		vkDestroyImageView(vulkan_device, swapchain_images_views[i], NULL);
		vkDestroyFramebuffer(vulkan_device, swapchain_frame_buffers[i], NULL);
		vkDestroyFence(vulkan_device, in_flight_fence[i], NULL);
	}
	
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(vulkan_device, image_available_semaphore[i], NULL);
		vkDestroySemaphore(vulkan_device, render_finished_semaphore[i], NULL);
	}

	vkDestroyCommandPool(vulkan_device, command_pool, NULL);
	vkDestroyPipeline(vulkan_device, graphics_pipeline, NULL);
	vkDestroyPipelineLayout(vulkan_device, pipeline_layout, NULL);
	vkDestroyRenderPass(vulkan_device, render_pass, NULL);
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
