#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;

class VulkanTextureCUBE
{
public:
	VulkanTextureCUBE();
	~VulkanTextureCUBE();

	void								CreateTextureCUBE(VulkanDevice* pDevice, std::string fileName);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	void								CreateTextureImage(VulkanDevice* pDevice, std::string fileName);
	void								CreateTextureSampler(VulkanDevice* pDevice);
	
public:
	VkImage								m_vkTextureImage;
	VkImageView							m_vkTextureImageView;
	VkImageLayout						m_vkTextureImageLayout;
	VkDeviceMemory						m_vkTextureImageMemory;
	VkSampler							m_vkTextureSampler;
	VkDescriptorImageInfo				m_vkTextureImageDescriptorInfo;
};

