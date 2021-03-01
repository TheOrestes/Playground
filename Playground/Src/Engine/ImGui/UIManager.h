#pragma once

#include "vulkan/vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanFrameBuffer;

class UIManager
{
public:
	~UIManager();

	static UIManager& getInstance()
	{
		static UIManager manager;
		return manager;
	}

	void						Initialize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkRenderPass renderPass);
	//void						RecordCommands(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VulkanFrameBuffer* pFrameBuffer, VkRenderPass renderPass, uint32_t currentImage);

	void						HandleWindowResize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void						Cleanup(VulkanDevice* pDevice);
	void						CleanupOnWindowResize(VulkanDevice* pDevice);

	void						BeginRender();
	void						EndRender();

private:
	UIManager();
	UIManager(const UIManager&);
	void operator=(const UIManager&);

	void						InitDescriptorPool(VulkanDevice* pDevice);
	//void						InitRenderPass(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

private:
	VkDescriptorPool			m_vkDescriptorPool;
	//VkRenderPass				m_vkRenderPass;
	//VulkanFrameBuffer*			m_pFramebuffer;
};

