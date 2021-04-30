#pragma once

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

class VulkanDevice;
class VulkanSwapChain;
class VulkanGraphicsPipeline;
class VulkanTexture2D;
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
struct HDRIShaderPushData
{
	HDRIShaderPushData()
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

	void																 CreateTextureCUBE(VulkanDevice* pDevice, std::string fileName);
	void																 CreateTextureCubeFromHDRI(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, std::string fileName);
	void																 CreateIrradianceCUBE(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, uint32_t dimension);
	void																 Cleanup(VulkanDevice* pDevice);
	void																 CleanupOnWindowResize(VulkanDevice* pDevice);
																		 
private:															
	void																 CreateTextureImage(VulkanDevice* pDevice, std::string fileName);
	VkSampler															 CreateTextureSampler(VulkanDevice* pDevice);

	// Generic, HDRI->Cubemap, CubeMap->IrradMap
	VkRenderPass														 CreateOffscreenRenderPass(VulkanDevice* pDevice, VkFormat format);
	std::tuple<VkImage, VkImageView, VkDeviceMemory, VkFramebuffer>		 CreateOffscreenFramebuffer(VulkanDevice* pDevice, VkRenderPass renderPass, VkFormat format, uint32_t dimension);

	// HDRI->Cubemap Generation Pass!
	std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> CreateHDRI2CubeDescriptorSet(VulkanDevice* pDevice);
	void																 CreateHDRI2CubePipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkDescriptorSetLayout descSetLayout, VkRenderPass renderPass);
	void																 RenderHDRI2CUBE(VulkanDevice* pDevice, VkRenderPass renderPass, VkFramebuffer framebuffer,
																						 VkImage irradImage, VkImage offscreenImage, VkDescriptorSet irradDescSet, uint32_t dimension);

	// Cubemap->Irradiance Map Generation Pass!
	std::tuple<VkDescriptorPool, VkDescriptorSetLayout, VkDescriptorSet> CreateIrradianceDescriptorSet(VulkanDevice* pDevice);
	void																 CreateIrradiancePipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkDescriptorSetLayout descSetLayout, VkRenderPass renderPass);
	void																 RenderIrrandianceCUBE(VulkanDevice* pDevice, VkRenderPass renderPass, VkFramebuffer framebuffer,
																							   VkImage irradImage, VkImage offscreenImage, VkDescriptorSet irradDescSet, uint32_t dimension);

public:	
	// HDRI Image
	VulkanTexture2D*													 m_pTextureHDRI;
	VulkanGraphicsPipeline*												 m_pGraphicsPipelineHDRI2Cube;

	// Cubemap															 
	VkImage																 m_vkImageCUBE;
	VkImageView															 m_vkImageViewCUBE;
	VkImageLayout														 m_vkImageLayoutCUBE;
	VkDeviceMemory														 m_vkImageMemoryCUBE;
	VkSampler															 m_vkSamplerCUBE;
																		 
	// Irradiance Map													 
	VkImage																 m_vkImageIRRAD;
	VkImageView															 m_vkImageViewIRRAD;
	VkImageLayout														 m_vkImageLayoutIRRAD;
	VkDeviceMemory														 m_vkImageMemoryIRRAD;
	VkSampler															 m_vkSamplerIRRAD;
	VulkanGraphicsPipeline*												 m_pGraphicsPipelineIrradiance;
																		 
	DummySkybox*														 m_pDummySkybox;																	
};

