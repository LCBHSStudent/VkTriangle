#pragma once

/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
//-------------------明天注意下这块-----------------------------
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> 
		getAttributeDescriptions() {
		
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
//----------------------------------------------------//
};

static struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 &&
			presentFamily >= 0;
	}
};

struct UniformBufferObject {
	glm::mat4		model;
	glm::mat4		view;
	glm::mat4		proj;
};

class VkApp {
public:
	explicit	 VkApp() = default;
	virtual void Run();
	bool         isDeviceSuitable(VkPhysicalDevice);

	static void  framebufferResizeCallback(GLFWwindow*, int, int);
	static void  getBindingDescription();

	static const size_t MAX_FRAMES_IN_FLIGHT = 2;

	static struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;

		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>	presentModes;
	};

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
	
	void _CreateVertexBuffers();
	void _CreateIndicesBuffer();
	void _CreateUniformBuffers();

	void _CreateDescriptorSetLayout();
	void _CreateGraphicsPipeline();
	void _CreateCommandBuffers();
	void _CreateSyncObjects();
	void _CreateSemaphores();			//废弃,改用上面的SyncObjCreateFunc

	VkShaderModule
		_CreateShaderModule(const std::vector<char>&);

	bool _CheckValidationLayersSupport(
		const std::vector<const char*>& validationLayers);
	bool _CheckDeviceExtensionSupport(VkPhysicalDevice device);

	std::vector<const char*>
		_GetRequiredExtensions();

	QueueFamilyIndices
		findQueueFamilies(VkPhysicalDevice device);

	uint32_t findMemoryType(uint32_t typeFilter,
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
	VkPipelineLayout         m_pipelineLayout	 {};
	VkRenderPass             m_renderPass        {};
	VkPipeline               m_graphicsPipeline  {};

	VkBuffer				 m_vertexBuffer      {};
	VkDeviceMemory           m_vertexBufferMemory{};
	VkCommandPool			 m_commandPool       {};

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
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};
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
