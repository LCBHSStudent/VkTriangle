
/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#include <set>
#include "VkApp.h"
#include <algorithm>
#include <exception>
#include "../LCBHSS/lcbhss_space.h"

void VkApp::Run() {
#ifdef _DEBUG
	EnableLogging();
#else
	DisableLogging();
#endif

	try {
		_InitWindow();
	}
	catch (const char* err) {
		std::cerr << err << std::endl;
		LOG_AND_EXIT(CREATE_WINDOW_FAILED);
	}

	try {
		_InitVulkan();
	}
	catch (std::runtime_error err) {
		std::cerr << err.what() << std::endl;
		LOG_AND_EXIT(CREATE_VK_INSTANCE_FAILED);
	}
	
	_exec();
	_CleanUp();
}

bool VkApp::isDeviceSuitable(VkPhysicalDevice device) {
	
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures   deviceFeatures;

	vkGetPhysicalDeviceProperties(
		device, &deviceProperties
	);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	
	bool queueJudge = findQueueFamilies(device).isComplete();
	bool extensionJudge = _CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;

	if (extensionJudge) {
		SwapChainSupportDetails swapChainSupport =
			querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() &&
			!swapChainSupport.presentModes.empty();
	}

	return deviceProperties.deviceType ==
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		deviceFeatures.geometryShader &&
		queueJudge &&
		extensionJudge &&
		swapChainAdequate;
}

//----------------------------------------//
//---------------INIT VULKAN--------------//
int VkApp::_InitVulkan() {

	_CreateInstance();
	_CreateSurface();

#ifdef _DEBUG
	_SetupDebugCallback();
#endif

	try {
		_PickPhysicalDevice();
	}
	catch (std::runtime_error err) {
		Log(err.what());
	}
	
	_CreateLogicalDevice();
	_CreateSwapChain();
	_CreateImageViews();
	_CreateRenderPass();
	_CreateGraphicsPipeline();
	_CreateFramebuffers();
	_CreateCommandPool();
	_CreateVertexBuffers();
	_CreateCommandBuffers();
	_CreateSyncObjects();

	return 0;
}
//----------------------------------------//
//----------------------------------------//

void VkApp::_SetupDebugCallback() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = 
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	// 指定回调函数处理消息的级别
	createInfo.messageSeverity =	
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData	   = nullptr;

	if (_CreateDebugUtilsMessengerEXT(
		m_instance, &createInfo, nullptr, &m_callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback");
	}
}

VkApp::QueueFamilyIndices VkApp::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int index = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = index;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = index;
		}

		if (indices.isComplete()) {
			break;
		}

		index++;
	}

	return indices;
}

uint32_t VkApp::findMemoryType(
	uint32_t typeFilter, VkMemoryPropertyFlags properties
) {

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(
		m_gpu, &memProperties
	);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
			((memProperties.memoryTypes[i].propertyFlags & properties) == 
			properties)
		) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

VkApp::SwapChainSupportDetails VkApp::querySwapChainSupport(
	VkPhysicalDevice device
) {
	SwapChainSupportDetails details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device, m_surface, &details.capabilities
	);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device, m_surface, &formatCount, nullptr
	);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, m_surface, &formatCount, details.formats.data()
		);
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device, m_surface, &presentModeCount, nullptr
	);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, m_surface, &presentModeCount, details.presentModes.data()
		);
	}

	return details;
}


void VkApp::_PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(
		m_instance, &deviceCount, devices.data()
	);

	{
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				m_gpu = device;
				break;
			}
		}
		if (m_gpu == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU");
		}
	}
}

void VkApp::_CreateLogicalDevice() {

	QueueFamilyIndices indices = findQueueFamilies(m_gpu);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = {
		indices.graphicsFamily, indices.presentFamily
	};

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType =
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = 
		static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(
		deviceExtensions.size()
	);
	createInfo.ppEnabledExtensionNames =
		deviceExtensions.data();



#ifdef _DEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(
		validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else 
	createInfo.enabledLayerCount = 0;
#endif // _DEBUG

	if (vkCreateDevice(m_gpu, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
		throw std::runtime_error("create device failed");
	}

	vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
}

void VkApp::_CreateSurface() {
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface)
		!= VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
}

void VkApp::_CreateSwapChain() {

	SwapChainSupportDetails swapChainSupport =
		querySwapChainSupport(m_gpu);

	VkSurfaceFormatKHR surfaceFormat =
		chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode =
		chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D	extent =
		chooseSwapExtent(swapChainSupport.capabilities);

	// 通过设置交换链中的图像个数，通过获取交换链支持的最小图像个数+1来实现
	// 三倍缓冲
	
	uint32_t imageCount =
		swapChainSupport.capabilities.minImageCount + 1;
		// maxImageCount == 0 表示可以使用内存许可下的任意张图像
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	// 如果要对图像处理等后期操作，使用VK_IMAGE_USAGE_TRANSFER_DST_BIT
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	QueueFamilyIndices indices = findQueueFamilies(m_gpu);
	uint32_t queueFamilyIndices[] = {
		(uint32_t)indices.graphicsFamily,
		(uint32_t)indices.presentFamily
	};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode =
			VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode =
			VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	// 允许VK产生一定的优化措施
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(
		m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS
		) {
		throw std::runtime_error("failed to create swap chain");
	}

	
	vkGetSwapchainImagesKHR(
		m_device, m_swapChain, &imageCount, nullptr
	);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(
		m_device, m_swapChain, &imageCount, swapChainImages.data()
	);

	swapChainExtent = extent;
	swapChainImageFormat = surfaceFormat.format;

}

void VkApp::_CreateImageViews() {

	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType =
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		// --此处决定了图像数据的解释方式-- //
		createInfo.viewType =
			VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		
		//设置对图像颜色通道的映射，此处为默认映射
		createInfo.components.r = 
			VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = 
			VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = 
			VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = 
			VK_COMPONENT_SWIZZLE_IDENTITY;

		// -- subResourceRange 设置了指定图像的用途和图像的那一部分
		// 可以用于访问（此处由于图像被用作渲染目标且没有细分级别
		// 所以只存在一个图层
		createInfo.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		// 若编写VR类的应用程序，可能会使用支持多个层次的交换链
		// 为每个图像创建多个图像视图来访问左眼和右眼饿图层
		if (vkCreateImageView(
			m_device, &createInfo, nullptr, &swapChainImageViews[i]
		) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image view");
		}
	}

}

void VkApp::_CreateRenderPass() {

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format			= swapChainImageFormat;
	colorAttachment.samples			= VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType =
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(
		m_device, &renderPassInfo, nullptr, &m_renderPass
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
}

void VkApp::_CreateFramebuffers() {
	swapChainFrameBuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType =
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(
			m_device, &framebufferInfo,
			nullptr, &swapChainFrameBuffers[i]
		) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void VkApp::_CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(
		m_gpu
	);
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType =
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0;

	if (vkCreateCommandPool(
		m_device, &poolInfo, nullptr, &m_commandPool
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool");
	}

}

void VkApp::_CreateVertexBuffers() {
	
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType =
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(vertices[0]) * vertices.size();
	bufferInfo.usage =
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode =
		VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(
		m_device, &bufferInfo, nullptr, &m_vertexBuffer
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(
		m_device, m_vertexBuffer, &memRequirements
	);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType =
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;

	allocInfo.memoryTypeIndex = findMemoryType(
		memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	if (vkAllocateMemory(
		m_device, &allocInfo, nullptr, &m_vertexBufferMemory
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory");
	}
	vkBindBufferMemory(
		m_device, m_vertexBuffer, m_vertexBufferMemory, 0
	);
	//此处的offset需要被memRequirements.alignment整除
	void* data;
	vkMapMemory(
		m_device, m_vertexBufferMemory, 0, bufferInfo.size, 0, &data
	);
	memcpy(data, vertices.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(
		m_device, m_vertexBufferMemory
	);

}

void VkApp::_CreateGraphicsPipeline() {

	auto vertShaderCode = readFile(
		"A:/WorkSpace/CppProject/VkForVs/VkForVs/shaders/vert.spv");
	auto fragShaderCode = readFile(
		"A:/WorkSpace/CppProject/VkForVs/VkForVs/shaders/frag.spv");

	VkShaderModule vertShaderModule =
		_CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule =
		_CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage =
		VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	// pName指定了调用着色器的函数
	vertShaderStageInfo.pName = "main";
	//
	//**** pSpecializtionInfo 可以通过它来之地那个着色器用到的
	//**** 常量，可以对同一个着色器模块对象指定不同的着色器常量
	//**** 用于管线的创建，是的编译器可以根据指定的着色器常量
	//**** 消除一些条件分支 比在渲染时使用变量配置着色器效率高得多
	//
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage =
		VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderStageInfo, fragShaderStageInfo
	};


	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//--------------------------Set bind description--------------------------//
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount =
		static_cast<uint32_t>(attributeDescriptions.size());

	vertexInputInfo.pVertexBindingDescriptions =
		&bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions =
		attributeDescriptions.data();
//-----------------------------------------------------------------------//
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology =
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //三个顶点构成一个图元
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width  = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	
	// 设置为true对生成贴图的阴影很有用（在两个平面外的片段不会被直接丢弃
	rasterizer.depthClampEnable = VK_FALSE;
	// 所有图元都不能通过光栅化
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	// 光栅化后的线宽, > 1.0f 时需要启用gpu特性
	rasterizer.lineWidth = 1.0f;
	// 表面剔除 
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	// 指定顺时针的顶点序是正面
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamping = {};
	multisamping.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamping.sampleShadingEnable = VK_FALSE;
	multisamping.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamping.minSampleShading = 1.0f;
	multisamping.pSampleMask = nullptr;
	multisamping.alphaToCoverageEnable = VK_FALSE;
	multisamping.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(
		m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType =
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState	= &inputAssembly;
	pipelineInfo.pViewportState			= &viewportState;
	pipelineInfo.pRasterizationState	= &rasterizer;
	pipelineInfo.pMultisampleState		= &multisamping;
	pipelineInfo.pDepthStencilState		= nullptr;
	pipelineInfo.pColorBlendState		= &colorBlending;
	pipelineInfo.pDynamicState			= nullptr;

	pipelineInfo.layout					= m_pipelineLayout;
	pipelineInfo.renderPass				= m_renderPass;
	pipelineInfo.subpass				= 0;

	pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex		= -1;

	if (vkCreateGraphicsPipelines(
		m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}

	vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

}

void VkApp::_CreateCommandBuffers() {

	m_commandBuffers.resize(swapChainFrameBuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(
		m_device, &allocInfo, m_commandBuffers.data()
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers");
	}

	for (size_t i = 0; i < m_commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags =
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(
			m_commandBuffers[i], &beginInfo
		) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType =
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = swapChainFrameBuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(
			m_commandBuffers[i],
			&renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE); //不需要辅助缓冲
	//--------------------绑定 buffers---------------------------//
		vkCmdBindPipeline(m_commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_graphicsPipeline
		);
		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(
			m_commandBuffers[i], 0, 1, vertexBuffers, offsets
		);
	//----------------------------------------------------------//
		//	size()个顶点  一个渲染实例（为1表示不进行实例渲染） firstVertex firstInstance
		//											     	|||        |||
		//                                          gl_VertexIndex gl_InstanceIndex
		vkCmdDraw(
			m_commandBuffers[i], static_cast<uint32_t>(vertices.size()),
			1, 0, 0
		);

		vkCmdEndRenderPass(m_commandBuffers[i]);
		if (vkEndCommandBuffer(
			m_commandBuffers[i]
		) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}
	}

}

void VkApp::_CreateSyncObjects() {
	
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType =
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType =
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags =
		VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(
				m_device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]
			) != VK_SUCCESS ||
			vkCreateSemaphore(
				m_device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]
			) != VK_SUCCESS ||
			vkCreateFence(
				m_device, &fenceInfo, nullptr, &inFlightFences[i]
			) != VK_SUCCESS
		) {
			throw std::runtime_error("failed to create "
				"synchronization objects for a frame");
		}
	}

}

void VkApp::_CreateSemaphores() {

	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType =
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(
				m_device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]
			) != VK_SUCCESS ||
			vkCreateSemaphore(
				m_device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]
			) != VK_SUCCESS
		) {
			throw std::runtime_error("failed to create semaphores for a frame");
		}
	}

}

VkShaderModule VkApp::_CreateShaderModule(
	const std::vector<char>& code
) {

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(
		code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(
		m_device, &createInfo, nullptr, &shaderModule
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module");
	}
	return shaderModule;
}

int VkApp::_CreateInstance() {

#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif
	if (enableValidationLayers &&
		!_CheckValidationLayersSupport(validationLayers)
	) {
		throw std::runtime_error("validation layers "
			"requested but not supported");
	}
	
	VkApplicationInfo appInfo   = {};
	appInfo.sType			    = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName    = "Vk Demo";
	appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName		    = "LunarG SDK";
	appInfo.apiVersion		    = VK_API_VERSION_1_1; //1_1?

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions =
		glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount   = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount	   = 0;
//-------------------启用校验层---------------------------
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(
			validationLayers.size()
		);
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}
//----------------启用glfw debug utils---------------------
	auto _glfwExtensions = _GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(
		_glfwExtensions.size()
	);
	createInfo.ppEnabledExtensionNames = _glfwExtensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create instance");
	}
//-----------------打印vk支持的扩展信息-----------------------
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(
		nullptr, &extensionCount, nullptr
	);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(
		nullptr, &extensionCount, extensions.data()
	);
#ifdef _DEBUG
	std::cerr << "Available extensions:" << std::endl;
	for (const auto& extension : extensions) {
		std::cerr << "\t" << extension.extensionName << std::endl;
	}
#endif

	return 0;
}

bool VkApp::_CheckValidationLayersSupport (
	const std::vector<const char*>& validationLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> avaliableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(
		&layerCount, avaliableLayers.data()
	);

	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		
		for (const auto& layerProperties : avaliableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

bool VkApp::_CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, nullptr
	);
	
	std::vector<VkExtensionProperties> availableExtensions(
		extensionCount
	);
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, availableExtensions.data()
	);
	std::set<std::string> requiredExtensions(
		deviceExtensions.begin(), deviceExtensions.end()
	);
	
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(
			extension.extensionName
		);
	}

	return requiredExtensions.empty();
}

void VkApp::framebufferResizeCallback(GLFWwindow* window,
	int width, int height) {
	auto app = reinterpret_cast<VkApp*> (
		glfwGetWindowUserPointer(window)
	);
	app->frameBufferResized = true;
}

void VkApp::getBindingDescription() {

}

int VkApp::_InitWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(
		W_WIDTH, W_HEIGHT, "Vulkan Demo", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window,
		framebufferResizeCallback
	);
	
	if (m_window == nullptr) {
		throw "Create window failed";
	}

	return 0;
}

int VkApp::_exec() {

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		_drawFrame();
	}

	vkDeviceWaitIdle(m_device);

	return 0;
}

void VkApp::_drawFrame() {

	// 使用栅栏缓冲来等待执行完毕
	vkWaitForFences (
		m_device, 1, &inFlightFences[m_curFrame],
		VK_TRUE, std::numeric_limits<uint64_t>::max()
	);

	// 获取帧图像对象
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		m_device, m_swapChain, std::numeric_limits<uint64_t>::max(),
		imageAvailableSemaphores[m_curFrame], VK_NULL_HANDLE, &imageIndex
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR ||
		result == VK_SUBOPTIMAL_KHR ||
		frameBufferResized
	) {
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = {
				imageAvailableSemaphores[m_curFrame]
			};
			VkPipelineStageFlags waitStages[] = {
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 0;
			submitInfo.pCommandBuffers = nullptr;

			if (vkQueueSubmit(
				m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE
			) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit draw command buffer");
			}
		}
		_resetSwapChain();

		return;
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to acquire swap chain image");
	}

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(m_device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[imageIndex] = inFlightFences[m_curFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {
		imageAvailableSemaphores[m_curFrame]
	};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = {
		renderFinishedSemaphores[m_curFrame]
	};

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(
		m_device, 1, &inFlightFences[m_curFrame]
	);

	if (vkQueueSubmit(
		m_graphicsQueue, 1, &submitInfo, inFlightFences[m_curFrame]
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;
	vkQueuePresentKHR(
		m_presentQueue,
		&presentInfo
	);

	m_curFrame = (m_curFrame + 1) & MAX_FRAMES_IN_FLIGHT;

}
void VkApp::_cleanUpSwapChain() {

	for (auto framebuffer : swapChainFrameBuffers) {
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(
		m_device, m_commandPool,
		static_cast<uint32_t>(m_commandBuffers.size()),
		m_commandBuffers.data()
	);

	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	vkDestroyRenderPass(m_device, m_renderPass, nullptr);

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(
			m_device, imageView, nullptr
		);
	}

	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

}

void VkApp::_resetSwapChain() {

	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}
	
	vkDeviceWaitIdle(m_device);

	_cleanUpSwapChain();

	_CreateSwapChain();
	_CreateImageViews();
	_CreateRenderPass();
	_CreateGraphicsPipeline();
	_CreateFramebuffers();
	_CreateCommandBuffers();

	frameBufferResized = false;
}

int VkApp::_CleanUp() {

#ifdef _DEBUG
	try {
		_DestroyDebugUtilsMessengerEXT(
			m_instance,
			m_callback,
			nullptr
		);
	}
	catch (std::runtime_error err) {
		Log(err.what());
		LOG_AND_EXIT(UNHANDLED_ERROR);
	}
#endif

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_device, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_device, inFlightFences[i], nullptr);
	}
	
	_cleanUpSwapChain();

	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

	try {
		vkDestroyInstance(m_instance, nullptr);
	}
	catch (...) {
		Log("Destroy instance failed");
		LOG_AND_EXIT(UNHANDLED_ERROR);
	}

	try {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		glfwTerminate();
	}
	catch (...) {
		Log("Destroy GLFW window failed");
		LOG_AND_EXIT(UNHANDLED_ERROR);
	}

	return 0;
}

std::vector<const char*> VkApp::_GetRequiredExtensions() {

#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(
		&glfwExtensionCount
	);

	std::vector<const char*> extensions(glfwExtensions,
		glfwExtensions + glfwExtensionCount
	);

	if (enableValidationLayers) {
		extensions.push_back(
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messgaeSeverity,
	VkDebugUtilsMessageTypeFlagsEXT		   messageType,
	const
	VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	/*if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		...display message if it's enough to show
	}*/
	std::cerr
		<< "validation layer: " << pCallbackData->pMessage 
		<< std::endl;

	return VK_FALSE;
}

VkSurfaceFormatKHR VkApp::chooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats
) {
	if (availableFormats.size() == 1 &&
		availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return {
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			availableFormat.colorSpace == 
				VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
			) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR VkApp::chooseSwapPresentMode(
	const std::vector<VkPresentModeKHR> availablePresentModes
) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto availablePresentMode :
		availablePresentModes) {
		// 三倍缓冲综合表现最佳，避免了撕裂现象同时具有较低的延迟
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		else if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	// 只有FIFO队列提交模式能保证一直可用
	return bestMode;
	// 如果呈现模式不可用，必须使用VK_PRESENT_MODE_IMMEDIATE_KHR
}

VkExtent2D VkApp::chooseSwapExtent(
	const VkSurfaceCapabilitiesKHR& capabilities
) {
	if (capabilities.currentExtent.width !=
		std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width,
				actualExtent.width)
		);
		actualExtent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height,
				actualExtent.height)
		);

		return actualExtent;
	}
	return VkExtent2D();
}


#ifdef _DEBUG

VkResult VkApp::_CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback
) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VkApp::_DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT callback,
	const VkAllocationCallbacks* pAllocator
) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

#endif