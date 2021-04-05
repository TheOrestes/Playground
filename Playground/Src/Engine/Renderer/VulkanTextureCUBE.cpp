#include "PlaygroundPCH.h"
#include "VulkanTextureCUBE.h"

#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanGraphicsPipeline.h"
#include "Engine/RenderObjects/DummySkybox.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"

#include "stb_image.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanTextureCUBE::VulkanTextureCUBE()
{
	m_vkImageCUBE					= VK_NULL_HANDLE;
	m_vkImageViewCUBE				= VK_NULL_HANDLE;
	m_vkImageMemoryCUBE				= VK_NULL_HANDLE;
	m_vkSamplerCUBE					= VK_NULL_HANDLE;

	m_pGraphicsPipelineIrradiance	= nullptr;
	m_pDummySkybox					= nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
VulkanTextureCUBE::~VulkanTextureCUBE()
{
	SAFE_DELETE(m_pGraphicsPipelineIrradiance);
	SAFE_DELETE(m_pDummySkybox);
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
void VulkanTextureCUBE::CreateIrradianceRenderPass(VulkanDevice* pDevice, VkFormat format)
{
	VkAttachmentDescription attachDesc = {};

	// Color attachment
	attachDesc.format						= format;
	attachDesc.samples						= VK_SAMPLE_COUNT_1_BIT;
	attachDesc.loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDesc.storeOp						= VK_ATTACHMENT_STORE_OP_STORE;
	attachDesc.stencilLoadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDesc.stencilStoreOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDesc.initialLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	attachDesc.finalLayout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference	colorAttachRef	= {};
	colorAttachRef.attachment				= 0;
	colorAttachRef.layout					= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription	subpassDesc		= {};
	subpassDesc.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount		= 1;
	subpassDesc.pColorAttachments			= &colorAttachRef;

	// Subpass dependencies
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass				= VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass				= 0;
	dependencies[0].srcStageMask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask			= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags			= VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass				= 0;
	dependencies[1].dstSubpass				= VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask			= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask			= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags			= VK_DEPENDENCY_BY_REGION_BIT;

	// Renderpass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount	= 1;
	renderPassCreateInfo.pAttachments		= &attachDesc;
	renderPassCreateInfo.subpassCount		= 1;
	renderPassCreateInfo.pSubpasses			= &subpassDesc;
	renderPassCreateInfo.dependencyCount	= 2;
	renderPassCreateInfo.pDependencies		= dependencies.data();

	VkResult result = vkCreateRenderPass(pDevice->m_vkLogicalDevice, &renderPassCreateInfo, nullptr, &m_vkRenderPassIRRAD);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Renderpass for Irradiance Cubemap creation!");
		return;
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateOffscreenFramebuffer(VulkanDevice* pDevice, VkFormat format, uint32_t dimension)
{
	// Color attachment
	m_vkImageOffscreen = Helper::Vulkan::CreateImage(pDevice, dimension, dimension, format, VK_IMAGE_TILING_OPTIMAL,
														 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
														 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryOffscreen);

	m_vkImageViewOffscreen = Helper::Vulkan::CreateImageView(pDevice, m_vkImageOffscreen, format, VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Framebuffer
	VkFramebufferCreateInfo fbCreateInfo = {};
	fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbCreateInfo.renderPass = m_vkRenderPassIRRAD;
	fbCreateInfo.attachmentCount = 1;
	fbCreateInfo.pAttachments = &m_vkImageViewOffscreen;
	fbCreateInfo.width = dimension;
	fbCreateInfo.height = dimension;
	fbCreateInfo.layers = 1;

	VkResult result = vkCreateFramebuffer(pDevice->m_vkLogicalDevice, &fbCreateInfo, nullptr, &m_vkFramebufferIRRAD);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create offscreen framebuffer for Irradiance map!");
		return;
	}

	Helper::Vulkan::TransitionImageLayout(pDevice, m_vkImageOffscreen, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateIrradianceDescriptorSet(VulkanDevice* pDevice)
{
	VkResult result;

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 1> arrDescriptorPoolSize = {};
	arrDescriptorPoolSize[0].descriptorCount = 1;
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = arrDescriptorPoolSize.size();
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();
	poolCreateInfo.maxSets = 2;

	result = vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &m_vkDescPoolIRRAD);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool for Irradiance map!");
		return;
	}

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 1> arrDescriptorSetLayoutBindings = {};
	arrDescriptorSetLayoutBindings[0].binding = 0;
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();
	
	result = vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &m_vkDescLayoutIRRAD);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set Layout for Irradiance map!");
		return;
	}

	// *** Create Descriptor Set
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkDescPoolIRRAD;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &m_vkDescLayoutIRRAD;

	result = vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, &m_vkDescSetIRRAD);
	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Set for Irradiance map!");
		return;
	}

	// *** Write Descriptor Sets (Cubemap Image goes in as parameter to shader which then gets converted into Irrad Map!)
	VkDescriptorImageInfo cubemapImageInfo = {};
	cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											
	cubemapImageInfo.imageView = m_vkImageViewCUBE;																	
	cubemapImageInfo.sampler = m_vkSamplerCUBE;			

	// Descriptor write info
	VkWriteDescriptorSet cubemapSetWrite = {};
	cubemapSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	cubemapSetWrite.dstSet = m_vkDescSetIRRAD;
	cubemapSetWrite.dstBinding = 0;
	cubemapSetWrite.dstArrayElement = 0;
	cubemapSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubemapSetWrite.descriptorCount = 1;
	cubemapSetWrite.pImageInfo = &cubemapImageInfo;

	// Update the descriptor sets with new buffer/binding info
	vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, 1, &cubemapSetWrite, 0, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateIrradiancePipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	m_pGraphicsPipelineIrradiance = new VulkanGraphicsPipeline(PipelineType::IRRADIANCE_CUBE, pSwapchain);

	// Descriptor Set layouts!
	std::vector<VkDescriptorSetLayout> setLayouts = { m_vkDescLayoutIRRAD };

	// Push constants!
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(IrradShaderPushData);

	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	m_pGraphicsPipelineIrradiance->CreatePipelineLayout(pDevice, setLayouts, pushConstantRanges);
	m_pGraphicsPipelineIrradiance->CreateGraphicsPipeline(pDevice, pSwapchain, m_vkRenderPassIRRAD, 0, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::RenderIrrandianceCUBE(VulkanDevice* pDevice, uint32_t dimension)
{
	std::array<VkClearValue, 1> arrClearValues;
	arrClearValues[0].color = { {0.0f, 0.0f, 0.2f, 0.0f} };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkRenderPassIRRAD;
	renderPassBeginInfo.framebuffer = m_vkFramebufferIRRAD;
	renderPassBeginInfo.renderArea.extent.width = dimension;
	renderPassBeginInfo.renderArea.extent.height = dimension;
	renderPassBeginInfo.clearValueCount = arrClearValues.size();
	renderPassBeginInfo.pClearValues = arrClearValues.data();

	std::vector<glm::mat4> matrices = 
	{
		// POSITIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	VkViewport vp = {};
	vp.width = (float)dimension;
	vp.height = (float)dimension;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	VkRect2D scissorRect = {};
	scissorRect.extent.width = dimension;
	scissorRect.extent.height = dimension;
	scissorRect.offset.x = 0.0f;
	scissorRect.offset.y = 0.0f;

	IrradShaderPushData irradPushData;

	// Start recording commands to command buffer!
	VkCommandBuffer cmdBuffer = pDevice->BeginCommandBuffer();

	vkCmdSetViewport(cmdBuffer, 0, 1, &vp);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissorRect);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;

	// Change image layout for all cubemap faces to transfer destination
	Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, m_vkImageIRRAD, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

	// 6 faces of irradiance map!
	for (uint32_t f = 0; f < 6; f++)
	{
		// Render scene from cube face's point of view
		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		irradPushData.mvp = glm::perspective((float)(M_PI_OVER_TWO), 1.0f, 0.1f, 512.0f) * matrices[f];
		vkCmdPushConstants(	cmdBuffer,
							m_pGraphicsPipelineIrradiance->m_vkPipelineLayout,
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							0, sizeof(IrradShaderPushData),
							&irradPushData);	

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineIrradiance->m_vkGraphicsPipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineIrradiance->m_vkPipelineLayout, 0, 1, &m_vkDescSetIRRAD, 0, NULL);

		m_pDummySkybox->Render(pDevice, cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, m_vkImageOffscreen, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Copy region for transfer from framebuffer to cube face
		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = f;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };

		copyRegion.extent.width = static_cast<uint32_t>(vp.width);
		copyRegion.extent.height = static_cast<uint32_t>(vp.height);
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(cmdBuffer, m_vkImageOffscreen, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkImageIRRAD, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,&copyRegion);

		// Transform framebuffer color attachment back
		Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, m_vkImageOffscreen, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	// Transform Irradiance map to Shader readable optimal format!
	Helper::Vulkan::TransitionImageLayout(pDevice, cmdBuffer, m_vkImageIRRAD, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

	pDevice->EndAndSubmitCommandBuffer(cmdBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::CreateIrradianceCUBE(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, uint32_t dimension)
{
	m_pDummySkybox = new DummySkybox();
	m_pDummySkybox->Init(pDevice);

	const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

	//*** Create VkImage with 6 array Layers!
	m_vkImageIRRAD = Helper::Vulkan::CreateImageCUBE(pDevice,
													dimension,
													dimension,
													format,
													VK_IMAGE_TILING_OPTIMAL,
													VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vkImageMemoryIRRAD);

	// Create Image View!
	m_vkImageViewIRRAD = Helper::Vulkan::CreateImageViewCUBE(pDevice,
															m_vkImageIRRAD,
															format,
															VK_IMAGE_ASPECT_COLOR_BIT);

	// Create Sampler!
	m_vkSamplerIRRAD = CreateTextureSampler(pDevice);

	// Create RenderPass
	CreateIrradianceRenderPass(pDevice, format);

	// Create Off-screen framebuffer! 
	CreateOffscreenFramebuffer(pDevice, format, dimension);

	// Create Descriptor Set layout & Descriptor Set!
	CreateIrradianceDescriptorSet(pDevice);

	// Create Graphics Pipeline!
	CreateIrradiancePipeline(pDevice, pSwapchain);

	// Render
	RenderIrrandianceCUBE(pDevice, dimension);

	// Cleanup!
	vkDestroyRenderPass(pDevice->m_vkLogicalDevice, m_vkRenderPassIRRAD, nullptr);
	vkDestroyFramebuffer(pDevice->m_vkLogicalDevice, m_vkFramebufferIRRAD, nullptr);
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewIRRAD, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageIRRAD, nullptr);
	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewOffscreen, nullptr);
	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageOffscreen, nullptr);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescPoolIRRAD, nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, m_vkDescLayoutIRRAD, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanTextureCUBE::Cleanup(VulkanDevice* pDevice)
{
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerCUBE, nullptr);
	vkDestroySampler(pDevice->m_vkLogicalDevice, m_vkSamplerIRRAD, nullptr);

	vkDestroyImageView(pDevice->m_vkLogicalDevice, m_vkImageViewCUBE, nullptr);

	vkDestroyImage(pDevice->m_vkLogicalDevice, m_vkImageCUBE, nullptr);

	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryCUBE, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryIRRAD, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkImageMemoryOffscreen, nullptr);

	m_pGraphicsPipelineIrradiance->Cleanup(pDevice);
	m_pDummySkybox->Cleanup(pDevice);
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

