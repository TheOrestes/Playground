#pragma once

#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "glm/glm.hpp"
#include "vulkan/vulkan.h"

#include "Engine/Helpers/Log.h"

namespace Helper
{
	namespace App
	{
		const uint32_t MAX_FRAME_DRAWS = 2;
		const uint32_t MAX_OBJECTS = 20;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Read shader file...
		inline std::vector<char> ReadShaderFile(const std::string& fileName)
		{
			// start reading at the end & in binary mode.
			// Advantage of reading file from the end is we can use read position to determine
			// size of the file & allocate buffer accordingly!
			std::ifstream file(fileName, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				LOG_ERROR("Failed to open Shader file!");

			// get the file size & allocate buffer memory!
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);

			// now seek back to the beginning of the file & read all bytes at once!
			file.seekg(0);
			file.read(buffer.data(), fileSize);

			// close the file & return bytes!
			file.close();

			return buffer;
		}

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
		//--- Create shader module
		inline VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<char>& shaderCode)
		{
			VkShaderModuleCreateInfo shaderModuleInfo;
			shaderModuleInfo.codeSize = shaderCode.size();
			shaderModuleInfo.flags = 0;
			shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
			shaderModuleInfo.pNext = nullptr;
			shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS)
				LOG_ERROR("Failed to create shader module!");

			return shaderModule;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Find suitable memory type based on allowed type & property flags
		inline uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypeIndex, VkMemoryPropertyFlags props)
		{
			// Get properties of physical device memory
			VkPhysicalDeviceMemoryProperties memoryProps;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);

			for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++)
			{
				if ((allowedTypeIndex & (1 << i))											// Index of memory type must match corresponding bit in allowed types!
					&& (memoryProps.memoryTypes[i].propertyFlags & props) == props)			// Desired property bit flags are part of the memory type's property flags!
				{
					// This memory type is valid, so return index!
					return i;
				}
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Create VkBuffer & VkDeviceMemory of specific size, based on usage flags & property flags. 
		inline void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags,
			VkMemoryPropertyFlags bufferProperties, VkBuffer* outBuffer, VkDeviceMemory* outBufferMemory)
		{

			// Information to create a buffer (doesn't include assigning memory)
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;													// total size of buffer
			bufferInfo.usage = bufferUsageFlags;											// type of buffer
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;								// similar to swap chain images, can share vertex buffers

			if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, outBuffer) != VK_SUCCESS)
				LOG_ERROR("Failed to create Vertex Buffer");

			// GET BUFFER MEMORY REQUIREMENTS
			VkMemoryRequirements	memRequirements;
			vkGetBufferMemoryRequirements(logicalDevice, *outBuffer, &memRequirements);

			// ALLOCATE MEMORY TO BUFFER
			VkMemoryAllocateInfo memoryAllocInfo = {};
			memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocInfo.allocationSize = memRequirements.size;
			memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,			// Index of memory type on Physical Device that has required bit flags
				bufferProperties);

			// Allocated memory to VkDeviceMemory
			if (vkAllocateMemory(logicalDevice, &memoryAllocInfo, nullptr, outBufferMemory) != VK_SUCCESS)
				LOG_ERROR("Failed to allocated Vertex Buffer Memory!");

			// Allocate memory to given Vertex buffer
			vkBindBufferMemory(logicalDevice, *outBuffer, *outBufferMemory, 0);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Begin Command buffer for recording commands! 
		inline VkCommandBuffer BeginCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool)
		{
			// Command buffer to hold transfer command
			VkCommandBuffer commandBuffer;

			// Command buffer details
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = commandPool;
			allocInfo.commandBufferCount = 1;

			// Allocate command buffer from pool
			vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

			// Information to begin the command buffer record!
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// We are only using the command buffer once, so set for one time submit!

			// Begin recording transfer commands
			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			return commandBuffer;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- End recording Commands & submit them to the Queue!
		inline void EndAndSubmitCommandBuffer(VkDevice logicalDevice, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
		{
			// End Commands!
			vkEndCommandBuffer(commandBuffer);

			// Queue submission information
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// Submit transfer command to transfer queue (which is same as Graphics Queue) & wait until it finishes!
			vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue);

			// Free temporary command buffer back to pool!
			vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Generic Copy buffer from srcBuffer to dstBuffer using transferQueue & transferCommandPool of specific size
		inline void CopyBuffer(VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
			VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
		{
			// Create buffer
			VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(logicalDevice, transferCommandPool);

			// Region of data to copy from and to 
			VkBufferCopy bufferCopyRegion = {};
			bufferCopyRegion.srcOffset = 0;
			bufferCopyRegion.dstOffset = 0;
			bufferCopyRegion.size = bufferSize;

			// Command to copy src buffer to dst buffer
			vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

			EndAndSubmitCommandBuffer(logicalDevice, transferCommandPool, transferQueue, transferCommandBuffer);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Generic Copy Image buffer from srcBuffer to VkImage using transferQueue & transferCommandPool of specific width-height!
		inline void CopyImageBuffer(VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
			VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
		{
			//Create buffer
			VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(logicalDevice, transferCommandPool);

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

			EndAndSubmitCommandBuffer(logicalDevice, transferCommandPool, transferQueue, transferCommandBuffer);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Function to transition image layout from old to new image layout using command pool for VkImage!
		inline void TransitionImageLayout(VkDevice logicalDevice, VkQueue queue, VkCommandPool commandPool, VkImage image,
			VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
		{
			VkCommandBuffer commandBuffer = BeginCommandBuffer(logicalDevice, commandPool);

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

			EndAndSubmitCommandBuffer(logicalDevice, commandPool, queue, commandBuffer);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Create VkImage & VkDeviceMemory based on width-height-format-tiling-usageFlags-propertyFlags!
		inline VkImage CreateImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
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
			if (vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS)
				LOG_ERROR("Failed to create an image");

			// Get memory requirements for type of image
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(logicalDevice, image, &memoryRequirements);

			// Allocated memory using image requirements & user defined properties
			VkMemoryAllocateInfo memoryAllocInfo = {};
			memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocInfo.allocationSize = memoryRequirements.size;
			memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, propFlags);

			if (vkAllocateMemory(logicalDevice, &memoryAllocInfo, nullptr, imageMemory) != VK_SUCCESS)
				LOG_ERROR("Failed to allocated memory for image!");

			// Connect memory to image
			vkBindImageMemory(logicalDevice, image, *imageMemory, 0);

			return image;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//--- Create VkImageView for a VkImage
		inline VkImageView CreateImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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
			if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
				LOG_ERROR("Failed to create an Image View");

			return imageView;
		}
	}
}
