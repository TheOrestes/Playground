#pragma once

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

class VulkanDevice;
class VulkanSwapChain;
class VulkanGraphicsPipeline;
class DummySkybox;

//---------------------------------------------------------------------------------------------------------------------
struct IrradShaderPushData
{
	IrradShaderPushData()
	{
		mvp = glm::mat4(1);
	}

	// Data Specific
	alignas(64) glm::mat4				mvp;
};

//---------------------------------------------------------------------------------------------------------------------
class VulkanTextureCUBE
{
public:
	VulkanTextureCUBE();
	~VulkanTextureCUBE();

	void												CreateTextureCUBE(VulkanDevice* pDevice, std::string fileName);
	void												CreateIrradianceCUBE(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, uint32_t dimension);
	void												Cleanup(VulkanDevice* pDevice);
	void												CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	void												CreateTextureImage(VulkanDevice* pDevice, std::string fileName);
	VkSampler											CreateTextureSampler(VulkanDevice* pDevice);

	// Irradiance Cubemap Pass!
	void												CreateIrradianceRenderPass(VulkanDevice* pDevice, VkFormat format);
	void												CreateOffscreenFramebuffer(VulkanDevice* pDevice, VkFormat format, uint32_t dimension);
	void												CreateIrradianceDescriptorSet(VulkanDevice* pDevice);
	void												CreateIrradiancePipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void												RenderIrrandianceCUBE(VulkanDevice* pDevice, uint32_t dimension);
	
public:
	// Cubemap
	VkImage												m_vkImageCUBE;
	VkImageView											m_vkImageViewCUBE;
	VkImageLayout										m_vkImageLayoutCUBE;
	VkDeviceMemory										m_vkImageMemoryCUBE;
	VkSampler											m_vkSamplerCUBE;

	// Irradiance Map
	VkImage												m_vkImageIRRAD;
	VkImageView											m_vkImageViewIRRAD;
	VkImageLayout										m_vkImageLayoutIRRAD;
	VkDeviceMemory										m_vkImageMemoryIRRAD;
	VkSampler											m_vkSamplerIRRAD;
	VulkanGraphicsPipeline*								m_pGraphicsPipelineIrradiance;

	DummySkybox*										m_pDummySkybox;

private:
	VkImage												m_vkImageOffscreen;
	VkImageView											m_vkImageViewOffscreen;
	VkDeviceMemory										m_vkImageMemoryOffscreen;
	VkRenderPass										m_vkRenderPassIRRAD;
	VkFramebuffer										m_vkFramebufferIRRAD;
	VkDescriptorPool									m_vkDescPoolIRRAD;
	VkDescriptorSetLayout								m_vkDescLayoutIRRAD;
	VkDescriptorSet										m_vkDescSetIRRAD;
};

