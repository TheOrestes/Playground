#pragma once

#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "glm/glm.hpp"
#include "vulkan/vulkan.h"

#include "Engine/Helpers/Log.h"
#include "Engine/Renderer/VulkanDevice.h"

namespace Helper
{
	namespace App
	{
		const uint32_t	MAX_FRAME_DRAWS = 2;
		const float WINDOW_WIDTH = 960.0f;
		const float WINDOW_HEIGHT = 540.0f;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Vertex data with Position
		struct VertexP
		{
			VertexP() { Position = glm::vec3(0); }
			VertexP(const glm::vec3& _pos) :
				Position(_pos) {}

			glm::vec3 Position;			// Vertex position X, Y, Z
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Vertex data with Position, Color
		struct VertexPC
		{
			VertexPC() { Position = glm::vec3(0);  Color = glm::vec3(0); }
			VertexPC(const glm::vec3& _pos, const glm::vec3& _col) :
				Position(_pos),
				Color(_col) {}

			glm::vec3 Position;			// Vertex position X, Y, Z
			glm::vec3 Color;			// Vertex Color R, G, B
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Vertex data with Position, UV
		struct VertexPT
		{
			VertexPT() { Position = glm::vec3(0);  UV = glm::vec2(0); }
			VertexPT(const glm::vec3& _pos, const glm::vec2& _uv) :
				Position(_pos),
				UV(_uv) {}

			glm::vec3 Position;			// Vertex position X, Y, Z
			glm::vec2 UV;				// Texture coordinates U,V
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Vertex data with Position, Color, UV
		struct VertexPCT
		{
			VertexPCT() { Position = glm::vec3(0);  Color = glm::vec3(0);  UV = glm::vec2(0); }
			VertexPCT(const glm::vec3& _pos, const glm::vec3& _color, const glm::vec2& _uv) :
				Position(_pos),
				Color(_color),
				UV(_uv) {}

			glm::vec3 Position;			// Vertex position X, Y, Z
			glm::vec3 Color;			// Vertex Color R, G, B
			glm::vec2 UV;				// Texture coordinates U,V
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Vertex data with Position, Normal, Tangent, BiNormal, Texcoords
		struct VertexPNTBT
		{
			VertexPNTBT() { Position = glm::vec3(0);  Normal = glm::vec3(0);  Tangent = glm::vec3(0); BiNormal = glm::vec3(0); UV = glm::vec2(0); }
			VertexPNTBT(const glm::vec3& _pos, const glm::vec3& _normal, const glm::vec3& _tangent, const glm::vec3& _binormal, const glm::vec2& _uv) :
				Position(_pos),
				Normal(_normal),
				Tangent(_tangent),
				BiNormal(_binormal),
				UV(_uv) {}

			glm::vec3 Position;			// Vertex position X, Y, Z
			glm::vec3 Normal;			// Normals
			glm::vec3 Tangent;			// Tangents
			glm::vec3 BiNormal;			// BiNormals
			glm::vec2 UV;				// Texture coordinates U,V
		};
	}


	//-----------------------------------------------------------------------------------------------------------------------
	namespace Vulkan
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- list of validation layers...
		const std::vector<const char*> g_strValidationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef _DEBUG
		const bool g_bEnableValidationLayer = true;
#else
		const bool g_bEnableValidationLayer = false;
#endif

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//-- list of device extensions...
		const std::vector<const char*> g_strDeviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Generic Copy Image buffer from srcBuffer to VkImage using transferQueue & transferCommandPool of specific width-height!
		inline void CopyImageBuffer(VulkanDevice* pDevice, VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
		{
			//Create buffer
			VkCommandBuffer transferCommandBuffer = pDevice->BeginCommandBuffer();

			VkBufferImageCopy imageRegion = {};
			imageRegion.bufferOffset = 0;											// offset into data
			imageRegion.bufferRowLength = 0;										// row length of data to calculate data spacing
			imageRegion.bufferImageHeight = 0;										// image height to calculate data spacing
			imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// which aspect of image to copy
			imageRegion.imageSubresource.mipLevel = 0;								// mipmap level to copy
			imageRegion.imageSubresource.baseArrayLayer = 0;						// starting array layer (if array)
			imageRegion.imageSubresource.layerCount = 1;							// number of layers to copy starting at baseArrayLayer
			imageRegion.imageOffset = { 0,0,0 };									// offset into image (as a opposed to raw data in the buffer)
			imageRegion.imageExtent = { width, height, 1 };							// size of region to copy as (x,y,z) values

			// copy buffer to given image
			vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

			pDevice->EndAndSubmitCommandBuffer(transferCommandBuffer);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Function to transition image layout from old to new image layout using command pool for VkImage!
		inline void TransitionImageLayout(VulkanDevice* pDevice, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
		{
			VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();

			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;								// Layout to transition from
			imageMemoryBarrier.newLayout = newImageLayout;								// Layout to transition to
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			imageMemoryBarrier.image = image;											// Image being accessed & modified as a part of barrier
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alteration on
			imageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from base mip level
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer of start alterations on
			imageMemoryBarrier.subresourceRange.layerCount = 1;							// Number of layers to alter starting from base array layer

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;

			// If transitioning from new image to image ready to receive data...
			if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				imageMemoryBarrier.srcAccessMask = 0;									// memory access stage transition must happen after this stage
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// memory access stage transition must happen before this stage

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			// If transitioning from transfer destination to shader readable...
			else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}

			vkCmdPipelineBarrier(commandBuffer,
				srcStage, dstStage,		// Pipeline stages (match to src & dest AcccessMask)
				0,							// Dependency flags
				0, nullptr,				// Memory barrier count + data
				0, nullptr,				// Buffer memory barrier count + data
				1, &imageMemoryBarrier);	// image memmory barrier count + data

			pDevice->EndAndSubmitCommandBuffer(commandBuffer);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Create VkImage & VkDeviceMemory based on width-height-format-tiling-usageFlags-propertyFlags!
		inline VkImage CreateImage(VulkanDevice* pDevice, uint32_t width, uint32_t height, VkFormat format,	VkImageTiling tiling, 
									VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
		{
			// Image creation info
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;							// Type of image
			imageCreateInfo.extent.width = width;									// width of image extent
			imageCreateInfo.extent.height = height;									// height of image extent
			imageCreateInfo.extent.depth = 1;										// depth of image ( just 1, no 3D aspect) 
			imageCreateInfo.mipLevels = 1;											// number of mipmap levels
			imageCreateInfo.arrayLayers = 1;										// number of levels in image array
			imageCreateInfo.format = format;										// format type of image	
			imageCreateInfo.tiling = tiling;										// how image data should be tiled
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// layout of image data on creation
			imageCreateInfo.usage = usageFlags;										// bit flags defining what image will be used for 
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;						// number of samples for multi-sampling
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;				// whether image can be shared between queues

			// Create Image
			VkImage image;
			if (vkCreateImage(pDevice->m_vkLogicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
				LOG_ERROR("Failed to create an image");

			// Get memory requirements for type of image
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(pDevice->m_vkLogicalDevice, image, &memoryRequirements);

			// Allocated memory using image requirements & user defined properties
			VkMemoryAllocateInfo memoryAllocInfo = {};
			memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocInfo.allocationSize = memoryRequirements.size;
			memoryAllocInfo.memoryTypeIndex = pDevice->FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, propFlags);

			if (vkAllocateMemory(pDevice->m_vkLogicalDevice, &memoryAllocInfo, nullptr, imageMemory) != VK_SUCCESS)
				LOG_ERROR("Failed to allocated memory for image!");

			// Connect memory to image
			vkBindImageMemory(pDevice->m_vkLogicalDevice, image, *imageMemory, 0);

			return image;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Create VkImageView for a VkImage
		inline VkImageView CreateImageView(const VulkanDevice* pDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = image;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = format;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			// Create image view & return it
			VkImageView imageView;
			if (vkCreateImageView(pDevice->m_vkLogicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
				LOG_ERROR("Failed to create an Image View");

			return imageView;
		}
	}
}
