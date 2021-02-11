#include "PlaygroundPCH.h"
#include "VulkanFrameBuffer.h"

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Engine/Helpers/Log.h"
#include "Engine/Helpers/Utility.h"
#include "PlaygroundHeaders.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanFrameBuffer::VulkanFrameBuffer()
{
	m_pColorAttachment = nullptr;
	m_pDepthAttachment = nullptr;
	m_pNormalAttachment = nullptr;

	m_vecFramebuffers.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanFrameBuffer::~VulkanFrameBuffer()
{
	SAFE_DELETE(m_pColorAttachment);
	SAFE_DELETE(m_pDepthAttachment);
	SAFE_DELETE(m_pNormalAttachment);

	m_vecFramebuffers.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CreateColorAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	m_pColorAttachment = new FramebufferAttachment();

	m_pColorAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
	m_pColorAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
	m_pColorAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

	std::vector<VkFormat> formats = { VK_FORMAT_R8G8B8A8_UNORM };
	m_pColorAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


	for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		// Create color buffer image
		m_pColorAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
			pSwapChain->m_vkSwapchainExtent.width,
			pSwapChain->m_vkSwapchainExtent.height,
			m_pColorAttachment->attachmentFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&(m_pColorAttachment->vecAttachmentImageMemory[i]));

		// Create color buffer image view!
		m_pColorAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
			m_pColorAttachment->vecAttachmentImage[i],
			m_pColorAttachment->attachmentFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CreateDepthAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	m_pDepthAttachment = new FramebufferAttachment();

	m_pDepthAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
	m_pDepthAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
	m_pDepthAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
	m_pDepthAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		// Create color buffer image
		m_pDepthAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
			pSwapChain->m_vkSwapchainExtent.width,
			pSwapChain->m_vkSwapchainExtent.height,
			m_pDepthAttachment->attachmentFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&(m_pDepthAttachment->vecAttachmentImageMemory[i]));

		// Create color buffer image view!
		m_pDepthAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
			m_pDepthAttachment->vecAttachmentImage[i],
			m_pDepthAttachment->attachmentFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CreateNormalAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	m_pNormalAttachment = new FramebufferAttachment();

	m_pNormalAttachment->vecAttachmentImage.resize(pSwapChain->m_vecSwapchainImages.size());
	m_pNormalAttachment->vecAttachmentImageView.resize(pSwapChain->m_vecSwapchainImages.size());
	m_pNormalAttachment->vecAttachmentImageMemory.resize(pSwapChain->m_vecSwapchainImages.size());

	std::vector<VkFormat> formats = { VK_FORMAT_R8G8B8A8_UNORM };
	m_pNormalAttachment->attachmentFormat = ChooseSupportedFormats(pDevice, formats, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);


	for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		// Create Normal buffer image
		m_pNormalAttachment->vecAttachmentImage[i] = Helper::Vulkan::CreateImage(pDevice,
			pSwapChain->m_vkSwapchainExtent.width,
			pSwapChain->m_vkSwapchainExtent.height,
			m_pNormalAttachment->attachmentFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&(m_pNormalAttachment->vecAttachmentImageMemory[i]));

		// Create normal buffer image view!
		m_pNormalAttachment->vecAttachmentImageView[i] = Helper::Vulkan::CreateImageView(pDevice,
			m_pNormalAttachment->vecAttachmentImage[i],
			m_pNormalAttachment->attachmentFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

//---------------------------------------------------------------------------------------------------------------------
VkFormat VulkanFrameBuffer::ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through options & find the compatible one
	for (VkFormat format : formats)
	{
		// Get properties for given formats on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(pDevice->m_vkPhysicalDevice, format, &properties);

		// depending on tiling choice, need to check for different bit flag
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}

		LOG_ERROR("Failed to find matching format!");
	}
}


//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, VkRenderPass renderPass)
{
	if (!pDevice || !pSwapChain)
		return;

	// resize framebuffer count to equal swap chain image views count
	m_vecFramebuffers.resize(pSwapChain->m_vecSwapchainImages.size());

	// create framebuffer for each swap chain image view
	for (uint32_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); ++i)
	{
		m_arrAttachments = { pSwapChain->m_vecSwapchainImageViews[i],
								m_pColorAttachment->vecAttachmentImageView[i],
								m_pDepthAttachment->vecAttachmentImageView[i],
								m_pNormalAttachment->vecAttachmentImageView[i] };


		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;											// Render pass layout the framebuffer will be used with					 
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(m_arrAttachments.size());
		framebufferCreateInfo.pAttachments = m_arrAttachments.data();							// List of attachments
		framebufferCreateInfo.width = pSwapChain->m_vkSwapchainExtent.width;					// framebuffer width
		framebufferCreateInfo.height = pSwapChain->m_vkSwapchainExtent.height;					// framebuffer height
		framebufferCreateInfo.layers = 1;														// framebuffer layers
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.pNext = nullptr;

		if (vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &framebufferCreateInfo, nullptr, &m_vecFramebuffers[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create Framebuffer");
		}
		else
			LOG_INFO("Framebuffer created!");
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::Cleanup(VulkanDevice* pDevice)
{
	m_pColorAttachment->Cleanup(pDevice);
	m_pDepthAttachment->Cleanup(pDevice);
	m_pNormalAttachment->Cleanup(pDevice);

	// Destroy frame buffers!
	for (uint32_t i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffers[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanFrameBuffer::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	m_pColorAttachment->CleanupOnWindowResize(pDevice);
	m_pDepthAttachment->CleanupOnWindowResize(pDevice);
	m_pNormalAttachment->CleanupOnWindowResize(pDevice);

	// Destroy frame buffers!
	for (uint32_t i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vecFramebuffers[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FramebufferAttachment::Cleanup(VulkanDevice* pDevice)
{
	// Cleanup depth related buffers, textures, memory etc.
	for (uint16_t i = 0; i < vecAttachmentImage.size(); i++)
	{
		vkDestroyImageView(pDevice->m_vkLogicalDevice, vecAttachmentImageView[i], nullptr);
		vkDestroyImage(pDevice->m_vkLogicalDevice, vecAttachmentImage[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecAttachmentImageMemory[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FramebufferAttachment::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	// Cleanup depth related buffers, textures, memory etc.
	for (uint16_t i = 0; i < vecAttachmentImage.size(); i++)
	{
		vkDestroyImageView(pDevice->m_vkLogicalDevice, vecAttachmentImageView[i], nullptr);
		vkDestroyImage(pDevice->m_vkLogicalDevice, vecAttachmentImage[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecAttachmentImageMemory[i], nullptr);
	}
}
