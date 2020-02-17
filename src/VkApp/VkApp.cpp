
/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#include "VkApp.h"
#include "../LCBHSS/lcbhss_space.h"

#include <set>
#include <chrono>
#include <algorithm>
#include <exception>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
		deviceFeatures.samplerAnisotropy &&
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

	
	_CreateDescriptorSetLayout();		//描述符集布局
	_CreateGraphicsPipeline();
	_CreateFramebuffers();
	_CreateCommandPool();				//需要指令缓冲的命令的基础

	_CreateTextureImage();
	_CreateTextureSampler();

	_CreateVertexBuffers();
	_CreateIndicesBuffer();
	_CreateUniformBuffers();
	_CreateDescriptorPool();
	_CreateDescriptorSets();
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

QueueFamilyIndices VkApp::findQueueFamilies(VkPhysicalDevice device) {
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

SwapChainSupportDetails VkApp::querySwapChainSupport(
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
	//----启用各向异性
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	
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
		swapChainImageViews[i] = 
			createImageView(swapChainImages[i], swapChainImageFormat);
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

void VkApp::_CreateTextureImage() {

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.png",
		&texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	
	VkDeviceSize imgSize = static_cast<size_t>(texWidth) 
		* static_cast<size_t>(texHeight) * 4;				//rgba

	if (!pixels) {
		throw std::runtime_error("failed to load texture image");
	}

	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemoey;

	_createBuffer( imgSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemoey
	);

	void* data;
	vkMapMemory(
		m_device, stagingBufferMemoey, 0, imgSize, 0, &data
	);
	memcpy(data, pixels, static_cast<size_t>(imgSize));
	vkUnmapMemory(
		m_device, stagingBufferMemoey
	);
	stbi_image_free(pixels);

	createImage(
		texWidth, texHeight,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage, textureImageMemory
	);
	
	transitionImageLayout(
		textureImage, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	copyBufferToImage(stagingBuffer, textureImage,
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight)
	);

	// 将图像转换为可呈现的模式
	transitionImageLayout(
		textureImage,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
	// 

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemoey, nullptr);

	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM);

}

void VkApp::_CreateTextureSampler() {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType =
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter =
		VK_FILTER_LINEAR;		//纹理放大
	samplerInfo.minFilter =
		VK_FILTER_LINEAR;		//纹理缩小
	// u,v,w => x,y,z轴
	samplerInfo.addressModeU =
		VK_SAMPLER_ADDRESS_MODE_REPEAT;		//超出图像范围后重复
	samplerInfo.addressModeV =
		VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW =
		VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable =			//各向异性
		VK_TRUE;
	samplerInfo.maxAnisotropy = 16;			//x16
	samplerInfo.borderColor =
		VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	//-----------------对不同分辨率纹理采用相同纹理坐标的模式：false
	//-----------------[0,1]模式
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	//-----------------阴影贴图时设置
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	//----分级细化（用于过滤操作）
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod	   = 0.0f;
	samplerInfo.maxLod	   = 0.0f;

	if (vkCreateSampler(
		m_device, &samplerInfo, nullptr, &textureSampler
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler");
	}

}

VkImageView VkApp::createImageView(VkImage image, VkFormat format) {

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	// --此处决定了图像数据的解释方式-- //
	createInfo.viewType =
		VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;

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

	VkImageView imageView;

	if (vkCreateImageView(
		m_device, &createInfo, nullptr, &imageView
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image view");
	}

	return imageView;
}

void VkApp::_CreateDescriptorSets() {

	std::vector<VkDescriptorSetLayout>
		layouts(swapChainImages.size(), m_descripSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(swapChainImages.size());
	if (vkAllocateDescriptorSets(
		m_device, &allocInfo, m_descriptorSets.data()
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		{
			bufferInfo.buffer = m_uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);
		}
		VkDescriptorImageInfo imgInfo = {};
		{
			imgInfo.imageLayout =
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imgInfo.imageView = textureImageView;
			imgInfo.sampler = textureSampler;
		}
		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
		{
			
			descriptorWrites[0].sType =
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;				//Binding 也是index？
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType =
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
		}
		{
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pImageInfo = nullptr;
			descriptorWrites[0].pTexelBufferView = nullptr;

			descriptorWrites[1].sType =
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet =
				m_descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imgInfo;
		}

		vkUpdateDescriptorSets(
			m_device, 
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0, nullptr
		);
	}

}

void VkApp::_CreateDescriptorPool() {
	
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].descriptorCount = static_cast<uint32_t>(
		swapChainImages.size());
	poolSizes[0].type =
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	
	poolSizes[1].descriptorCount = poolSizes[0].descriptorCount;
	poolSizes[1].type =
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = static_cast<uint32_t>(
		swapChainImages.size());

	if (vkCreateDescriptorPool(
		m_device, &createInfo, nullptr, &m_descriptorPool
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool");
	}

}

void VkApp::_CreateVertexBuffers() {

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	_createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	//此处的offset需要被memRequirements.alignment整除
	void* data;
	vkMapMemory(
		m_device, stagingBufferMemory, 0, bufferSize, 0, &data
	);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(
		m_device, stagingBufferMemory
	);

	_createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer,
		m_vertexBufferMemory
	);

	_copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);

}

void VkApp::_CreateIndicesBuffer() {
	
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	_createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory
	);

	void* data;
	vkMapMemory(
		m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(
		m_device, stagingBufferMemory
	);

	_createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indicesBuffer,
		m_indicesBufferMemory
	);

	_copyBuffer(stagingBuffer, m_indicesBuffer, bufferSize);

	vkDestroyBuffer(m_device, stagingBuffer, nullptr);
	vkFreeMemory(m_device, stagingBufferMemory, nullptr);

}

void VkApp::_CreateUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(swapChainImages.size());
	m_uniformBuffersMemory.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		_createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_uniformBuffers[i],
			m_uniformBuffersMemory[i]
		);
	}
}

void VkApp::_CreateDescriptorSetLayout() {

	VkDescriptorSetLayoutBinding uboLayoutBingding = {};
	{
		uboLayoutBingding.binding = 0; //对应的顶点着色器中的参数
		uboLayoutBingding.descriptorType =
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBingding.descriptorCount = 1;

		uboLayoutBingding.stageFlags =
			VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBingding.pImmutableSamplers = nullptr; //用于图像采样相关的描述符

	}
	
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	{
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
			uboLayoutBingding, samplerLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(
		m_device, &createInfo, nullptr, &m_descripSetLayout
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	}

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
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descripSetLayout;
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
		vkCmdBindIndexBuffer(m_commandBuffers[i], m_indicesBuffer,
			0, VK_INDEX_TYPE_UINT16);
	//----------------------------------------------------------//
		//	size()个顶点  一个渲染实例（为1表示不进行实例渲染） firstVertex firstInstance
		//											     	|||        |||
		//                                          gl_VertexIndex gl_InstanceIndex
		vkCmdBindDescriptorSets(m_commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout,
			0, 1, &m_descriptorSets[i], 0, nullptr);
		vkCmdDrawIndexed(
			m_commandBuffers[i], static_cast<uint32_t>(
				indices.size())
			, 1, 0, 0, 0
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

void VkApp::createImage(
	uint32_t width, uint32_t height,
	VkFormat format, VkImageTiling  tiling,
	VkImageUsageFlags				usage,
	VkMemoryPropertyFlags			properties,
	VkImage&						image,
	VkDeviceMemory&					imageMemory
) {
	VkImageCreateInfo imgInfo = {};
	imgInfo.sType =
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgInfo.imageType =
		VK_IMAGE_TYPE_2D;
	imgInfo.extent.width = width;
	imgInfo.extent.height = height;
	imgInfo.extent.depth = 1;
	imgInfo.mipLevels = 1;
	imgInfo.arrayLayers = 1;

	imgInfo.initialLayout =
		VK_IMAGE_LAYOUT_UNDEFINED;

	{
		imgInfo.format = format;			//图像库解析的像素数据格式
		VK_FORMAT_R8G8B8A8_UNORM;
		imgInfo.tiling = tiling;			// _OPTIMAL 以一种优化访问的方式排列
											// _LINEAR	行主序的方式排列(此处暂存缓冲不需要访问)
	
		imgInfo.usage = usage;				//需要被着色器采样
	}

	imgInfo.sharingMode =
		VK_SHARING_MODE_EXCLUSIVE;		//只被一个队列使用
	imgInfo.samples =
		VK_SAMPLE_COUNT_1_BIT;			//采样一次

	if (vkCreateImage(
		m_device, &imgInfo, nullptr, &image
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(
		memRequirements.memoryTypeBits,
		properties
	);

	if (vkAllocateMemory(
		m_device, &allocInfo, nullptr, &imageMemory
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory");
	}

	vkBindImageMemory(m_device, image, imageMemory, 0);
}

void VkApp::transitionImageLayout(
	VkImage image, VkFormat	format, 
	VkImageLayout oldLayout,
	VkImageLayout newLayout
) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType =
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	VkPipelineStageFlags srcStage;		// 未定义 -> 传输
	VkPipelineStageFlags dstStage;		// 传输   -> 着色器读取
	{
		
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask =
				VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage =
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage =
				VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				 (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL||
				  newLayout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR)
		) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage =
				VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage =
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::runtime_error("unsupported layout transition");
		}
	}

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;		//用于传递队列所有权
	barrier.image = image;
	barrier.subresourceRange.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.levelCount		= 1;
	barrier.subresourceRange.baseMipLevel	= 0;
	barrier.subresourceRange.layerCount		= 1;


	vkCmdPipelineBarrier(
		commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier
	);	//提交管线屏障对象

	endSingleTimeCommands(commandBuffer);
}

void VkApp::copyBufferToImage(
	VkBuffer			buffer, 
	VkImage				image, 
	uint32_t			width, 
	uint32_t			height
){
	VkCommandBuffer commandBuffer =
		beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;			//-+
	region.bufferImageHeight = 0;		//--存放方式：紧凑
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);

	endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VkApp::beginSingleTimeCommands() {

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType =
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level =
		VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(
		m_device, &allocInfo, &commandBuffer
	);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType =
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags =
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VkApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType =
		VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	
}

void VkApp::_createBuffer(
	VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory
) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType =
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer)
		!= VK_SUCCESS
	) {
		throw std::runtime_error("failed to create buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(
		m_device, buffer, &memRequirements
	);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType =
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(
		memRequirements.memoryTypeBits, properties
	);

	if (vkAllocateMemory(
		m_device, &allocInfo, nullptr, &bufferMemory
	) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory");
	}

	vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void VkApp::_copyBuffer(
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size
) {
	VkCommandBuffer commandBuffer =
		beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
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
		std::cerr << "window size changed, reset swap chain\n";
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

	_updateUniformBuffer(imageIndex);

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

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_descripSetLayout, nullptr);

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		vkDestroyBuffer(
			m_device, m_uniformBuffers[i], nullptr
		);
		vkFreeMemory(
			m_device, m_uniformBuffersMemory[i], nullptr
		);
	}

	vkDestroyImageView(m_device, textureImageView, nullptr);
	vkDestroyImage(m_device, textureImage, nullptr);
	vkFreeMemory(m_device, textureImageMemory, nullptr);
	vkDestroySampler(m_device, textureSampler, nullptr);

	vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
	vkDestroyBuffer(m_device, m_indicesBuffer, nullptr);
	vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
	vkFreeMemory(m_device, m_indicesBufferMemory, nullptr);
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


void VkApp::_updateUniformBuffer(uint32_t currentImage) {
	
	static auto startTime = std::chrono::high_resolution_clock
		::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time =
		std::chrono::duration<float, std::chrono::seconds::period> 
			(currentTime - startTime).count();
	
	UniformBufferObject ubo = {};
	// 按Z轴旋转Time弧度
	// rotate 矩阵 旋转角度 旋转轴 glm::mat4(1.0f) => 单位矩阵
	ubo.model = glm::rotate(glm::mat4(1.0f),
		time * glm::radians(90.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	// lookAt 观察者位置 视点坐标 向上向量为参数 视图变换矩阵
	ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	// 透视变换矩阵 视域的垂直角度，视域的宽高比 进平面远平面的距离
	ubo.proj = glm::perspective(
		glm::radians(45.0f),
		swapChainExtent.width / (float)swapChainExtent.height,
		0.1f, 10.0f
	);
	// GLM--OpenGL 与 vulkan的Y轴是相反的，所以*-1？
	// 但是注意这样做的话会使之前创建pipeline时设置的背面绘制此时变成顺时针，导致背面被剔除
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(
		m_device, m_uniformBuffersMemory[currentImage],
		0, sizeof(ubo), 0, &data
	);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(
		m_device,
		m_uniformBuffersMemory[currentImage]
	);

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