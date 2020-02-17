#pragma once

/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

#include "../VkAppDependence/vk_depend.h"

class VkApp {
public:
	explicit	 VkApp() = default;
	virtual void Run();
	bool         isDeviceSuitable(VkPhysicalDevice);

	static void  framebufferResizeCallback(GLFWwindow*, int, int);
	static void  getBindingDescription();

	static const size_t MAX_FRAMES_IN_FLIGHT = 2;

private:
	int  _exec();
	int  _InitVulkan();
	int  _InitWindow();
	void _drawFrame();
	int  _CleanUp();

	void _cleanUpSwapChain();
	void _resetSwapChain();
	void _updateUniformBuffer(uint32_t currentImage);
	
	void _createBuffer(
		VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory
	);
	void _copyBuffer (
		VkBuffer srcBuffer,
		VkBuffer dstBuffer,
		VkDeviceSize size
	);

    int  _CreateInstance();
	void _SetupDebugCallback();
	void _PickPhysicalDevice();
	void _CreateLogicalDevice();
	void _CreateSurface();
	void _CreateSwapChain();			//需要在窗口大小被改变的时候调用
	void _CreateImageViews();
	void _CreateRenderPass();
	void _CreateFramebuffers();

	void _CreateCommandPool();
	void _CreateTextureImage();
	void _CreateTextureSampler();

	void _CreateDescriptorSets();		// 描述符集
	void _CreateDescriptorPool();
	
	void _CreateVertexBuffers();
	void _CreateIndicesBuffer();
	void _CreateUniformBuffers();

	void _CreateDescriptorSetLayout();
	void _CreateGraphicsPipeline();
	void _CreateCommandBuffers();
	void _CreateSyncObjects();
	void _CreateSemaphores();			//废弃,改用上面的SyncObjCreateFunc
	
	VkCommandBuffer
		 beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);

	void createImage(
		uint32_t width, uint32_t height,
		VkFormat format, VkImageTiling		tiling,
		VkImageUsageFlags					usage,
		VkMemoryPropertyFlags				properties,
		VkImage&							image,
		VkDeviceMemory&						imageMemory
	);

	void transitionImageLayout(
		VkImage, VkFormat,
		VkImageLayout						oldLayout,
		VkImageLayout						newLayout
	);

	void copyBufferToImage(
		VkBuffer							buffer,
		VkImage								image,
		uint32_t							width,
		uint32_t							height
	);

	VkImageView
		createImageView(VkImage, VkFormat);
	
	VkShaderModule
		_CreateShaderModule(const std::vector<char>&);

	bool _CheckValidationLayersSupport(
		const std::vector<const char*>& validationLayers);
	bool _CheckDeviceExtensionSupport(VkPhysicalDevice device);

	std::vector<const char*>
		_GetRequiredExtensions();

	QueueFamilyIndices
		findQueueFamilies(VkPhysicalDevice device);

	uint32_t
		findMemoryType(uint32_t typeFilter,
		VkMemoryPropertyFlags properties);

	SwapChainSupportDetails
		querySwapChainSupport(VkPhysicalDevice device);

	static VKAPI_ATTR VkBool32 VKAPI_CALL
		debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messgaeSeverity,
			VkDebugUtilsMessageTypeFlagsEXT		   messageType,
			const
			VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector <VkPresentModeKHR>);
	VkExtent2D chooseSwapExtent(
		const VkSurfaceCapabilitiesKHR&);

	GLFWwindow*				 m_window;
	static const int		 W_WIDTH	  = 800;
	static const int		 W_HEIGHT	  = 600;
	bool                     frameBufferResized  = false;
	VkInstance				 m_instance			 {};
	VkPhysicalDevice		 m_gpu				 {};
	VkDevice				 m_device			 {};
	// 对于此处的queue，被隐式的指定为绘制和写入共用的队列
	VkQueue					 m_graphicsQueue	 {};
	VkQueue					 m_presentQueue		 {};
	VkSurfaceKHR			 m_surface	  = VK_NULL_HANDLE;
	VkSwapchainKHR			 m_swapChain		 {};
	VkFormat				 swapChainImageFormat{};
	VkExtent2D				 swapChainExtent	 {};

	VkDescriptorSetLayout    m_descripSetLayout  {};
	VkDescriptorPool		 m_descriptorPool{};
	std::vector<VkDescriptorSet>
							 m_descriptorSets    {};

	VkPipelineLayout         m_pipelineLayout	 {};
	VkRenderPass             m_renderPass        {};
	VkPipeline               m_graphicsPipeline  {};

	VkBuffer				 m_vertexBuffer      {};
	VkDeviceMemory           m_vertexBufferMemory{};
	VkCommandPool			 m_commandPool       {};
	
	VkImage					 textureImage;
	VkDeviceMemory			 textureImageMemory;
	VkImageView				 textureImageView;
	VkSampler				 textureSampler;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFrameBuffers;

	std::vector<VkCommandBuffer> m_commandBuffers;
	//--------------------------------------------------
	//-----------------Sync related---------------------
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence>     inFlightFences;
	std::vector<VkFence>	 imagesInFlight;
	size_t					 m_curFrame = 0;
	//--------------------------------------------------
	//--------------------------------------------------

	// --some configs-- //
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	}; 

	//--------------Vertex data-------------------//
	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	// update: 添加举行的四个顶点为纹理的坐标

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};
	VkBuffer	   m_indicesBuffer;
	VkDeviceMemory m_indicesBufferMemory;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	//--------------------------------------------//


#ifdef _DEBUG
	VkDebugUtilsMessengerEXT m_callback{};

	static VkResult _CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pCallback);

	static void _DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT callback,
		const VkAllocationCallbacks* pAllocator
	);
#endif
};
