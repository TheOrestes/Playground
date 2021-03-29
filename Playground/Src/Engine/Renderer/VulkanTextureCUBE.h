#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;

class VulkanTextureCUBE
{
public:
	VulkanTextureCUBE();
	~VulkanTextureCUBE();

	void								CreateTextureCUBE(VulkanDevice* pDevice, std::string fileName);
	void								CreateIrradianceCUBE(VulkanDevice* pDevice, uint32_t width, uint32_t height);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	void								CreateTextureImage(VulkanDevice* pDevice, std::string fileName);
	VkSampler							CreateTextureSampler(VulkanDevice* pDevice);
	
public:
	// Cubemap
	VkImage								m_vkImageCUBE;
	VkImageView							m_vkImageViewCUBE;
	VkImageLayout						m_vkImageLayoutCUBE;
	VkDeviceMemory						m_vkImageMemoryCUBE;
	VkSampler							m_vkSamplerCUBE;

	// Irradiance Map
	VkImage								m_vkImageIRRAD;
	VkImageView							m_vkImageViewIRRAD;
	VkImageLayout						m_vkImageLayoutIRRAD;
	VkDeviceMemory						m_vkImageMemoryIRRAD;
	VkSampler							m_vkSamplerIRRAD;
};

