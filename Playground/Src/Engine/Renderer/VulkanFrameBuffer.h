#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;

enum class AttachmentType
{
	FB_ATTACHMENT_ALBEDO,
	FB_ATTACHMENT_POSITION,
	FB_ATTACHMENT_NORMAL,
	FB_ATTACHMENT_DEPTH,
	FB_ATTACHMENT_REFLECTION,
	FB_ATTACHMENT_BACKGROUND
};

struct FramebufferAttachment
{
	FramebufferAttachment()
	{
		attachmentFormat = VK_FORMAT_UNDEFINED;
		vecAttachmentImage.clear();
		vecAttachmentImageView.clear();
		vecAttachmentImageMemory.clear();
	}

	void	Cleanup(VulkanDevice* pDevice);
	void	CleanupOnWindowResize(VulkanDevice* pDevice);
	
	VkFormat						attachmentFormat;
	std::vector<VkImage>			vecAttachmentImage;
	std::vector<VkImageView>		vecAttachmentImageView;
	std::vector<VkDeviceMemory>		vecAttachmentImageMemory;

	AttachmentType					attachmentType;
};

class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer();
	~VulkanFrameBuffer();

	void								CreateAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, AttachmentType eType);
	void								CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, VkRenderPass renderPass);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	VkFormat							ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats,
																VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	std::array<VkImageView, 4>			m_arrAttachments;

public:
	FramebufferAttachment*				m_pAlbedoAttachment;
	FramebufferAttachment*				m_pDepthAttachment;
	FramebufferAttachment*				m_pNormalAttachment;

	std::vector<VkFramebuffer>			m_vecFramebuffers;
};

