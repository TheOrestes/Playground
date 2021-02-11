#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;

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
};

class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer();
	~VulkanFrameBuffer();

	void							CreateColorAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain);
	void							CreateDepthAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain);
	void							CreateNormalAttachment(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain);

	void							CreateFrameBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, 
														VkRenderPass renderPass);

	void							Cleanup(VulkanDevice* pDevice);
	void							CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	VkFormat						ChooseSupportedFormats(VulkanDevice* pDevice, const std::vector<VkFormat>& formats,
															VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	std::array<VkImageView, 4>		m_arrAttachments;

public:
	FramebufferAttachment*			m_pColorAttachment;
	FramebufferAttachment*			m_pDepthAttachment;
	FramebufferAttachment*			m_pNormalAttachment;
	std::vector<VkFramebuffer>		m_vecFramebuffers;
};

