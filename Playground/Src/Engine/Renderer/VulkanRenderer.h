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

#include "IRenderer.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanFrameBuffer;
class VulkanGraphicsPipeline;

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
	void							CreateInstance();
	bool							CheckInstanceExtensionSupport(const std::vector<const char*>& instanceExtensions);
	bool							CheckValidationLayerSupport();
	void							SetupDebugMessenger();
	void							PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	
	void							HandleWindowResize();

	void							CreateSurface();
	void							CreateGraphicsPipeline();
	void							CreateRenderPass();
	void							CreateSyncObjects();

	void							CreateInputBuffersDescriptorPool();
	void							CreateInputBuffersDescriptorSets();

	void							RecordCommands(uint32_t currentImage);

	void							CleanupOnWindowResize();

	void							AllocateDynamicBufferTransferSpace();

private:
	GLFWwindow* m_pWindow;

	VkInstance						m_vkInstance;

	VulkanDevice*					m_pDevice;
	VulkanSwapChain*				m_pSwapChain;
	VulkanFrameBuffer*				m_pFrameBuffer;

	VulkanGraphicsPipeline*			m_pGraphicsPipelineOpaque;
	VulkanGraphicsPipeline*			m_pGraphicsPipelineDeferred;
	
	VkDebugUtilsMessengerEXT		m_vkDebugMessenger;
	VkSurfaceKHR					m_vkSurface;

	VkRenderPass					m_vkRenderPass;

	VkDescriptorPool				m_vkInputBuffersDescriptorPool;
	VkDescriptorSetLayout			m_vkInputBuffersDescriptorSetLayout;
	std::vector<VkDescriptorSet>	m_vecInputBuffersDescriptorSets;


	std::vector<VkSemaphore>		m_vecSemaphoreImageAvailable;
	std::vector<VkSemaphore>		m_vecSemaphoreRenderFinished;
	std::vector<VkFence>			m_vecFencesRender;
	uint32_t						m_uiCurrentFrame;

	bool							m_bFramebufferResized;

	// Scene Objects
	std::vector<Mesh>				m_vecMeshes;
	std::vector<Model*>				m_vecModels;

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

