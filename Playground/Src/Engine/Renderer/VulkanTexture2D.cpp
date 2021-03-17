#include "PlaygroundPCH.h"
#include "VulkanTexture2D.h"

#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanTexture2D::VulkanTexture2D()
{
	m_vkTextureImage				=	VK_NULL_HANDLE;
	m_vkTextureImageView			=	VK_NULL_HANDLE;
	m_vkTextureImageMemory			=	VK_NULL_HANDLE;
	m_vkTextureDeviceSize			=	VK_NULL_HANDLE;
	m_vkTextureSampler				=	VK_NULL_HANDLE;
	m_vkTextureImageDescriptorInfo	=	{};
}

//---------------------------------------------------------------------------------------------------------------------
VulkanTexture2D::~VulkanTexture2D()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTexture2D::CreateTexture(VulkanDevice* pDevice, std::string fileName, TextureType eType)
{
	// what is the type of this texture?
	m_eTextureType = eType;

	// Create texture image 
	CreateTextureImage(pDevice, fileName);

	// Create Image view 
	m_vkTextureImageView = Helper::Vulkan::CreateImageView(pDevice,
															m_vkTextureImage,
															VK_FORMAT_R8G8B8A8_UNORM,
															VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler
	CreateTextureSampler(pDevice);

	LOG_DEBUG("Created Vulkan Texture for {0}", fileName);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTexture2D::Cleanup(VulkanDevice* pDevice)
{
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkTextureSampler, nullptr);

	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkTextureImageView, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkTextureImage, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkTextureImageMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTexture2D::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}

//---------------------------------------------------------------------------------------------------------------------
unsigned char* VulkanTexture2D::LoadTextureFile(VulkanDevice* pDevice, std::string fileName)
{
	// Number of channels in image
	int channels = 0;

	std::string fileLoc = "Textures/" + fileName;

	// Load pixel data for an image
	unsigned char* imageData = stbi_load(fileLoc.c_str(), &m_iTextureWidth, &m_iTextureHeight, &m_iTextureChannels, STBI_rgb_alpha);
	if (!imageData)
	{
		LOG_ERROR(("Failed to load a Texture file! (" + fileName + ")").c_str());
	}

	// Calculate image size using given data
	m_vkTextureDeviceSize = m_iTextureWidth * m_iTextureHeight * 4;

	return imageData;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTexture2D::CreateTextureImage(VulkanDevice* pDevice, std::string fileName)
{
	stbi_uc* imageData = LoadTextureFile(pDevice, fileName);

	// Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;

	// create staging buffer to hold the loaded data, ready to copy to device!
	pDevice->CreateBuffer(m_vkTextureDeviceSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer,
		&imageStagingBufferMemory);

	// copy image data to staging buffer
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory, 0, m_vkTextureDeviceSize, 0, &data);
	memcpy(data, imageData, static_cast<uint32_t>(m_vkTextureDeviceSize));
	vkUnmapMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory);

	// Free original image data
	stbi_image_free(imageData);


	m_vkTextureImage = Helper::Vulkan::CreateImage(pDevice,
	                                               m_iTextureWidth,
	                                               m_iTextureHeight,
	                                               VK_FORMAT_R8G8B8A8_UNORM,
	                                               VK_IMAGE_TILING_OPTIMAL,
	                                               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkTextureImageMemory);

	// Transition image to be DST for copy operation
	Helper::Vulkan::TransitionImageLayout(pDevice,
		m_vkTextureImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// COPY DATA TO IMAGE
	Helper::Vulkan::CopyImageBuffer(pDevice, imageStagingBuffer, m_vkTextureImage, m_iTextureWidth, m_iTextureHeight);

	// Transition image to be shader readable for shader usage
	Helper::Vulkan::TransitionImageLayout(pDevice,
		m_vkTextureImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Destroy staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTexture2D::CreateTextureSampler(VulkanDevice* pDevice)
{
	//-- Sampler creation Info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;								// how to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;								// how to render when image is minified on screen			
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;			// how to handle texture wrap in U (x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;			// how to handle texture wrap in V (y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;			// how to handle texture wrap in W (z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;			// border beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;						// whether values of texture coords between [0,1] i.e. normalized
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;				// Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;										// Level of detail bias for mip level
	samplerCreateInfo.minLod = 0.0f;											// minimum level of detail to pick mip level
	samplerCreateInfo.maxLod = 0.0f;											// maximum level of detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_FALSE;								// Enable Anisotropy or not? Check physical device features to see if anisotropy is supported or not!
	samplerCreateInfo.maxAnisotropy = 16;										// Anisotropy sample level

	if (vkCreateSampler(pDevice->m_vkLogicalDevice, &samplerCreateInfo, nullptr, &m_vkTextureSampler) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Texture sampler!");
	}
}


