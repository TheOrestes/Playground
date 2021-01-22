#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "Engine\RenderObjects\Mesh.h"
#include "Engine\RenderObjects\Model.h"

#include "VulkanDevice.h"

#include "IRenderer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Once we know that swap-chains are available, we need to check its compatibility with our window surface. 
// We need to check following properties:
// 1. Surface capabilities (min-max number of images in swap chain, width-height of images)
// 2. Surface formats (pixel formats, color spaces)
// 3. Available presentation modes
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VulkanRenderer : public IRenderer
{
public:
	VulkanRenderer();
	virtual ~VulkanRenderer();

	virtual int						Initialize(GLFWwindow* pWindow) override;
	virtual void					Update(float dt) override;
	virtual void					Render() override;
	virtual void					Cleanup() override;

private:
	void							UpdateModel(int modelID, glm::mat4 modelMatrix);
	void							CreateInstance();
	bool							CheckInstanceExtensionSupport(const std::vector<const char*>& instanceExtensions);
	bool							CheckValidationLayerSupport();
	void							SetupDebugMessenger();
	void							PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void							CreateSurface();

	void							CreateLogicalDevice();

	SwapChainSupportDetails			QuerySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR				ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR				ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D						ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	VkFormat						ChooseSupportedFormats(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	void							CreateSwapChain();
	void							HandleWindowResize();
	void							CreateImageViews();
	void							CreateDescriptorSetLayout();
	void							CreatePushConstantRange();
	void							CreateGraphicsPipeline();
	void							CreateColorBufferImage();
	void							CreateDepthBufferImage();
	void							CreateRenderPass();
	void							CreateFramebuffers();
	void							CreateSyncObjects();
	void							CreateUniformBuffers();
	void							CreateDescriptorPool();
	void							CreateDescriptorSets();
	void							CreateInputDescriptorSets();

	unsigned char*					LoadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
	int								CreateTextureImage(std::string fileName);
	int								CreateTexture(std::string fileName);
	void							CreateTextureSampler();
	int								CreateTextureDescriptor(VkImageView textureImage);

	void							CreateModel(const std::string& fileName);

	void							UpdateUniformBuffers(uint32_t imageIndex);
	void							RecordCommands(uint32_t currentImage);

	void							CleanupOnWindowResize();

	void							AllocateDynamicBufferTransferSpace();

private:
	GLFWwindow* m_pWindow;

	VulkanDevice*					m_pDevice;

	VkInstance						m_vkInstance;
	QueueFamilyIndices				m_QueueFamilyIndices;

	VkDebugUtilsMessengerEXT		m_vkDebugMessenger;
	VkSurfaceKHR					m_vkSurface;

	VkDevice						m_vkDevice;

	VkSwapchainKHR					m_vkSwapchain;
	std::vector<VkImage>			m_vecSwapchainImages;
	VkFormat						m_vkSwapchainImageFormat;
	VkExtent2D						m_vkSwapchainExtent;

	VkRenderPass					m_vkRenderPass;

	VkPipelineLayout				m_vkPipelineLayout;
	VkPipeline						m_vkGraphicsPipeline;

	VkPipelineLayout				m_vkPipelineLayout2;
	VkPipeline						m_vkGraphicsPipeline2;

	std::vector<VkFramebuffer>		m_vecFramebuffers;
	std::vector<VkImageView>		m_vecSwapchainImageViews;
	std::vector<VkCommandBuffer>	m_vecCommandBuffer;

	VkFormat						m_vkDepthBufferFormat;
	std::vector<VkImage>			m_vecDepthBufferImage;
	std::vector<VkImageView>		m_vecDepthBufferImageView;
	std::vector<VkDeviceMemory>		m_vecDepthBufferImageMemory;

	VkFormat						m_vkColorBufferFormat;
	std::vector<VkImage>			m_vecColorBufferImage;
	std::vector<VkImageView>		m_vecColorBufferImageView;
	std::vector<VkDeviceMemory>		m_vecColorBufferImageMemory;

	std::vector<VkSemaphore>		m_vecSemaphoreImageAvailable;
	std::vector<VkSemaphore>		m_vecSemaphoreRenderFinished;
	std::vector<VkFence>			m_vecFencesRender;
	uint32_t						m_uiCurrentFrame;

	bool							m_bFramebufferResized;

	// - Descriptors
	VkDescriptorSetLayout			m_vkDescriptorSetLayout;
	VkDescriptorSetLayout			m_vkSamplerDescriptorSetLayout;
	VkDescriptorSetLayout			m_vkInputSeLayout;
	VkPushConstantRange				m_vkPushConstantRange;

	VkDescriptorPool				m_vkDescriptorPool;
	VkDescriptorPool				m_vkSamplerDescriptorPool;
	VkDescriptorPool				m_vkInputDescriptorPool;
	std::vector<VkDescriptorSet>	m_vecDescriptorSets;
	std::vector<VkDescriptorSet>	m_vecSamplerDescriptorSets;
	std::vector<VkDescriptorSet>	m_vecInputDescriptorSets;

	std::vector<VkBuffer>			m_vecVPUniformBuffer;
	std::vector<VkDeviceMemory>		m_vecVPUniformBufferMemory;

	std::vector<VkBuffer>			m_vecModelDynamicUniformBuffer;
	std::vector<VkDeviceMemory>		m_vecModelDynamicUniformBufferMemory;

	//VkDeviceSize					m_uiMinUniformBufferOffset;
	//uint32_t						m_uiModelUniformAlignment;
	//UBOModel*						m_pModelTransferSpace;


	// Scene Objects
	std::vector<Mesh>				m_vecMeshes;

	std::vector<VkImage>			m_vecTextureImages;
	std::vector<VkDeviceMemory>		m_vecTextureImageMemory;
	std::vector<VkImageView>		m_vecTextureImageViews;
	VkSampler						m_vkTextureSampler;

	std::vector<Model>				m_vecModels;

	// Scene Settings
	struct UBOViewProjection
	{
		glm::mat4 matProjection;
		glm::mat4 matView;
	}m_uboViewProjection;

	//-----------------------------------------------------------------------------------------------------------------
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		LOG_CRITICAL("Validation Layer: {0}", pCallbackData->pMessage);
		return VK_FALSE;
	}

	//-----------------------------------------------------------------------------------------------------------------
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	//-----------------------------------------------------------------------------------------------------------------
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	//-----------------------------------------------------------------------------------------------------------------
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto vkRenderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
		vkRenderer->m_bFramebufferResized = true;
	}
};

