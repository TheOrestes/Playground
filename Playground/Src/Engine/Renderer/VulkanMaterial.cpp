#include "PlaygroundPCH.h"
#include "VulkanMaterial.h"

#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "PlaygroundHeaders.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanMaterial::VulkanMaterial()
{
	m_vkDescriptorPool = VK_NULL_HANDLE;
	m_vkDescriptorSet = VK_NULL_HANDLE;
	m_vkDescriptorSetLayout = VK_NULL_HANDLE;

	m_mapTextures.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanMaterial::~VulkanMaterial()
{
	m_mapTextures.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::CreateDescriptorPool(VulkanDevice* pDevice)
{
	//--- SAMPLER DESCRIPTOR POOL
	// Texture sampler pool
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = m_mapTextures.size();

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = m_mapTextures.size();
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &samplerPoolCreateInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Sampler Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Sampler Descriptor Pool");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::CreateDescriptorSetLayout(VulkanDevice* pDevice)
{
	// TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
	// Texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	// Create a descriptor set layout with given bindings for texture
	VkDescriptorSetLayoutCreateInfo samplerLayoutCreateInfo = {};
	samplerLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	samplerLayoutCreateInfo.bindingCount = 1;
	samplerLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &samplerLayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a sampler descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Sampler Descriptor set layout");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::CreateDescriptorSets(VulkanDevice* pDevice, uint32_t descriptorBindingFlags)
{
	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkDescriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &m_vkDescriptorSetLayout;

	// allocate descriptor sets
	if (vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, &m_vkDescriptorSet) != VK_SUCCESS)
		LOG_ERROR("Failed to allocate Texture Descriptor Sets!");

	// Texture image info (for now we have only one albedo texture per material) 
	std::map<VulkanTexture*, TextureType>::iterator iter = m_mapTextures.begin();

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;				// Image layout when in use
	imageInfo.imageView = iter->first->m_vkTextureImageView;;						// image to bind to set
	imageInfo.sampler = iter->first->m_vkTextureSampler;							// sampler to use for the set

	// Descriptor write info
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = m_vkDescriptorSet;
	descriptorWrite.dstBinding = descriptorBindingFlags;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	// Update new descriptor set
	vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, 1, &descriptorWrite, 0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::LoadTexture(VulkanDevice* pDevice, const std::string& filePath, TextureType type)
{
	// Load texture based on it's type!
	switch (type)
	{
		case TextureType::TEXTURE_ALBEDO:
		{
			VulkanTexture* pTexture = new VulkanTexture();
			pTexture->CreateTexture(pDevice, filePath, type);

			m_mapTextures.emplace(pTexture, type);

			break;
		}
			
		case TextureType::TEXTURE_NORMAL:
			break;
		case TextureType::TEXTURE_SPECULAR:
			break;
		case TextureType::TEXTURE_ERROR:
			break;
		default:
			break;
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::Cleanup(VulkanDevice* pDevice)
{
	std::map<VulkanTexture*, TextureType>::iterator iter = m_mapTextures.begin();
	for (; iter != m_mapTextures.end(); ++iter)
	{
		iter->first->Cleanup(pDevice);
	}
	
	//vkFreeDescriptorSets(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, m_mapTextures.size(), &m_vkDescriptorSet);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, m_vkDescriptorSetLayout, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}
