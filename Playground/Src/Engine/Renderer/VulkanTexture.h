#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;

enum class TextureType
{
	TEXTURE_ALBEDO,
	TEXTURE_NORMAL,
	TEXTURE_SPECULAR,
	TEXTURE_ERROR
};

class VulkanTexture
{
public:
	VulkanTexture();
	~VulkanTexture();

	void								CreateTexture(VulkanDevice* pDevice, std::string fileName, TextureType eType);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

public:
	VkImage								m_vkTextureImage;
	VkImageView							m_vkTextureImageView;
	VkImageLayout						m_vkTextureImageLayout;
	VkDeviceMemory						m_vkTextureImageMemory;
	VkSampler							m_vkTextureSampler;
	VkDescriptorImageInfo				m_vkTextureImageDescriptorInfo;

private:
	unsigned char*						LoadTextureFile(VulkanDevice* pDevice, std::string fileName);
	void								CreateTextureImage(VulkanDevice* pDevice, std::string fileName);
	void								CreateTextureSampler(VulkanDevice* pDevice);

	int									m_iTextureWidth;
	int									m_iTextureHeight;
	int									m_iTextureChannels;
	VkDeviceSize						m_vkTextureDeviceSize;

	TextureType							m_eTextureType;
};
