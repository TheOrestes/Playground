#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanSwapChain;

enum class PipelineType
{
	GBUFFER_OPAQUE,
	FINAL_BEAUTY
};

class VulkanGraphicsPipeline
{
public:
	VulkanGraphicsPipeline(PipelineType type, VulkanSwapChain* pSwapChain);
	~VulkanGraphicsPipeline();

	void										CreatePipelineLayout(VulkanDevice* pDevice, const std::vector<VkDescriptorSetLayout>& layouts, 
																	VkPushConstantRange pushConstantRange);

	void										CreateGraphicsPipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, 
																		VkRenderPass renderPass, uint32_t subpass);

	void										Cleanup(VulkanDevice* pDevice);
	void										CleanupOnWindowResize(VulkanDevice* pDevice);

public:
	VkPipeline									m_vkGraphicsPipeline;
	VkPipelineLayout							m_vkPipelineLayout;

private:
	VkShaderModule								CreateShaderModule(VulkanDevice* pDevice, const std::string& fileName);

private:
	PipelineType								m_eType;
	
	std::string									m_strVertexShader;
	std::string									m_strFragmentShader;

	// Common parameters structures
	VkPipelineInputAssemblyStateCreateInfo		m_vkInputAssemblyInfo;
	VkViewport									m_vkViewportInfo;
	VkRect2D									m_vkScissorRect;
	VkPipelineViewportStateCreateInfo			m_vkViewportStateInfo;
	VkPipelineRasterizationStateCreateInfo		m_vkRasterizerCreateInfo;
	VkPipelineMultisampleStateCreateInfo		m_vkMultisamplingCreateInfo;
	VkPipelineDepthStencilStateCreateInfo		m_vkDepthStencilCreateInfo;
	VkPipelineColorBlendAttachmentState			m_vkColorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo			m_vkColorBlendStateCreateInfo;
	VkPipelineVertexInputStateCreateInfo		m_vkVertexInputStateCreateInfo;
};

