#pragma once

#include "vulkan/vulkan.h"
#include "Engine/Renderer/VulkanTexture.h"

class VulkanDevice;
class VulkanTexture;

class VulkanMaterial
{
public:
	VulkanMaterial();
	~VulkanMaterial();

	void					CreateDescriptorPool(VulkanDevice* pDevice);
	void					CreateDescriptorSetLayout(VulkanDevice* pDevice);
	void					CreateDescriptorSets(VulkanDevice* pDevice, uint32_t descriptorBindingFlags);
	void					LoadTexture(VulkanDevice* pDevice, const std::string& filePath, TextureType type);

private:
	std::map<VulkanTexture*, TextureType>	m_mapTextures;
	VkDescriptorPool		m_vkDescriptorPool;

public:
	VkDescriptorSet			m_vkDescriptorSet;
	VkDescriptorSetLayout	m_vkDescriptorSetLayout;
};

