#include "PlaygroundPCH.h"
#include "VulkanTextureCUBE.h"

#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"

#include "stb_image.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanTextureCUBE::VulkanTextureCUBE()
{
	m_vkImageCUBE				= VK_NULL_HANDLE;
	m_vkImageViewCUBE			= VK_NULL_HANDLE;
	m_vkImageMemoryCUBE			= VK_NULL_HANDLE;
	m_vkSamplerCUBE				= VK_NULL_HANDLE;
}

//---------------------------------------------------------------------------------------------------------------------
VulkanTextureCUBE::~VulkanTextureCUBE()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateTextureCUBE(VulkanDevice* pDevice, std::string fileName)
{
	// Create Texture image!
	CreateTextureImage(pDevice, fileName);

	// Create Image View!
	m_vkImageViewCUBE = Helper::Vulkan::CreateImageViewCUBE(	pDevice,
																m_vkImageCUBE,
																VK_FORMAT_R8G8B8A8_UNORM,
																VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler!
	m_vkSamplerCUBE = CreateTextureSampler(pDevice);
	
	LOG_DEBUG("Created Vulkan Cubemap Texture for {0}", fileName);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateIrradianceCUBE(VulkanDevice* pDevice, uint32_t width, uint32_t height)
{
	//*** Create VkImage with 6 array Layers!
	m_vkImageIRRAD = Helper::Vulkan::CreateImageCUBE(pDevice,
													width,
													height,
													VK_FORMAT_R32G32B32A32_SFLOAT,
													VK_IMAGE_TILING_OPTIMAL,
													VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryIRRAD);

	// Create Image View!
	m_vkImageViewIRRAD = Helper::Vulkan::CreateImageViewCUBE(pDevice,
															m_vkImageIRRAD,
															VK_FORMAT_R32G32B32A32_SFLOAT,
															VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler!
	m_vkSamplerIRRAD = CreateTextureSampler(pDevice);

	LOG_DEBUG("Created Vulkan Cubemap Irradiance Map");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::Cleanup(VulkanDevice* pDevice)
{
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerCUBE, nullptr);
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerIRRAD, nullptr);

	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewCUBE, nullptr);
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewIRRAD, nullptr);

	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageCUBE, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageIRRAD, nullptr);

	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryCUBE, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryIRRAD, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateTextureImage(VulkanDevice* pDevice, std::string fileName)
{
	// *** Load pixel data
	int width = 0, height = 0, channels = 0;

	std::array<std::string, 6> arrFileNames;

	arrFileNames[0] = "Textures/Cubemaps/" + fileName + '/' + "posx.jpg";
	arrFileNames[1] = "Textures/Cubemaps/" + fileName + '/' + "negx.jpg";
	arrFileNames[2] = "Textures/Cubemaps/" + fileName + '/' + "posy.jpg";
	arrFileNames[3] = "Textures/Cubemaps/" + fileName + '/' + "negy.jpg";
	arrFileNames[4] = "Textures/Cubemaps/" + fileName + '/' + "posz.jpg";
	arrFileNames[5] = "Textures/Cubemaps/" + fileName + '/' + "negz.jpg";

	// Data for 6 faces of cubemap!
	std::array<unsigned char*, 6> arrImageData = {};

	for (int i = 0; i < arrImageData.size(); ++i)
	{
		arrImageData[i] = stbi_load(arrFileNames[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!arrImageData[i])
		{
			LOG_ERROR(("Failed to load a Texture file! (" + arrFileNames[i] + ")").c_str());
			return;
		}
	}

	// Calculate image size using given data for 6 faces!
	const VkDeviceSize imageSize = width * height * 4 * 6;
	const VkDeviceSize layerSize = imageSize / 6;

	//*** Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;

	// create staging buffer to hold the loaded data, ready to copy to device!
	pDevice->CreateBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer,
		&imageStagingBufferMemory);

	//*** copy image data to staging buffer
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);

	VkDeviceSize currOffset = 0;
	for (int i = 0; i < arrImageData.size(); ++i)
	{
		memcpy(static_cast<unsigned char*>(data) + currOffset, arrImageData[i], layerSize);
		currOffset += layerSize;
	}

	vkUnmapMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory);

	// Free original image data
	for (int i = 0; i < arrImageData.size(); ++i)
	{
		stbi_image_free(arrImageData[i]);
	}

	//*** Create VkImage with 6 array Layers!
	m_vkImageCUBE = Helper::Vulkan::CreateImageCUBE(	pDevice,
														width,
														height,
														VK_FORMAT_R8G8B8A8_UNORM,
														VK_IMAGE_TILING_OPTIMAL,
														VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryCUBE);


	//*** Transition image to be DST for copy operation
	Helper::Vulkan::TransitionImageLayoutCUBE(	pDevice,
												m_vkImageCUBE,
												VK_IMAGE_LAYOUT_UNDEFINED,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// COPY DATA TO IMAGE
	Helper::Vulkan::CopyImageBufferCUBE(pDevice, imageStagingBuffer, m_vkImageCUBE, width, height);

	// Transition image to be shader readable for shader usage
	Helper::Vulkan::TransitionImageLayoutCUBE(	pDevice,
												m_vkImageCUBE,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//*** Destroy staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, imageStagingBufferMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
VkSampler VulkanTextureCUBE::CreateTextureSampler(VulkanDevice* pDevice)
{
	VkSampler sampler;

	//-- Sampler creation Info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;								// how to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;								// how to render when image is minified on screen			
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;		// how to handle texture wrap in U (x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;		// how to handle texture wrap in V (y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;		// how to handle texture wrap in W (z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;			// border beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;						// whether values of texture coords between [0,1] i.e. normalized
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;				// Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;										// Level of detail bias for mip level
	samplerCreateInfo.minLod = 0.0f;											// minimum level of detail to pick mip level
	samplerCreateInfo.maxLod = 0.0f;											// maximum level of detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_FALSE;								// Enable Anisotropy or not? Check physical device features to see if anisotropy is supported or not!
	samplerCreateInfo.maxAnisotropy = 16;										// Anisotropy sample level

	if (vkCreateSampler(pDevice->m_vkLogicalDevice, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Texture sampler!");
	}

	return sampler;
}
