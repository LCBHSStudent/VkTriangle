#pragma once

/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily  = -1;

	bool isComplete() {
		return graphicsFamily >= 0 &&
			presentFamily >= 0;
	}
};

struct SwapChainSupportDetails {

	VkSurfaceCapabilitiesKHR capabilities;

	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>	presentModes;

};

class VkApp {
public:
	explicit	 VkApp() = default;
	virtual void Run();
	bool         isDeviceSuitable(VkPhysicalDevice);

private:
	int  _InitVulkan();
	int  _InitWindow();
	int  _CleanUp();
	int  _exec();
	void _drawFrame();

    int  _CreateInstance();
	void _SetupDebugCallback();
	void _PickPhysicalDevice();
	void _CreateLogicalDevice();
	void _CreateSurface();
	void _CreateSwapChain();
	void _CreateImageViews();
	void _CreateRenderPass();
	void _CreateFramebuffers();
	void _CreateCommandPool();
	void _CreateGraphicsPipeline();
	void _CreateCommandBuffers();
	void _CreateSemaphores();

	VkShaderModule
		_CreateShaderModule(const std::vector<char>&);

	bool _CheckValidationLayersSupport(
		const std::vector<const char*>& validationLayers);
	bool _CheckDeviceExtensionSupport(VkPhysicalDevice device);

	std::vector<const char*>
		_GetRequiredExtensions();

	QueueFamilyIndices
		findQueueFamilies(VkPhysicalDevice device);

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


	GLFWwindow*				 m_window     = nullptr;
	static const int		 W_WIDTH	  = 800;
	static const int		 W_HEIGHT	  = 600;
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

	VkPipelineLayout         m_pipelineLayout	 {};
	VkRenderPass             m_renderPass        {};
	VkPipeline               m_graphicsPipeline  {};

	VkCommandPool			 m_commandPool       {};

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFrameBuffers;

	std::vector<VkCommandBuffer> m_commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	// --some configs-- //
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	}; 

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
