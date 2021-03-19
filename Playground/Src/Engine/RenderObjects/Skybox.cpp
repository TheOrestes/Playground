#include "PlaygroundPCH.h"
#include "Skybox.h"

#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanMaterial.h"
#include "Engine/Renderer/VulkanTextureCUBE.h"
#include "Engine/Renderer/VulkanGraphicsPipeline.h"
#include "Engine/Helpers/FreeCamera.h"


//---------------------------------------------------------------------------------------------------------------------
void SkyboxShaderUniforms::CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(SkyboxShaderUniforms);

	// one uniform buffer for each image (and by extension command buffer)
	vecBuffer.resize(pSwapchain->m_vecSwapchainImages.size());
	vecMemory.resize(pSwapchain->m_vecSwapchainImages.size());

	// create uniform buffers
	for (uint16_t i = 0; i < pSwapchain->m_vecSwapchainImages.size(); i++)
	{
		pDevice->CreateBuffer(vpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vecBuffer[i],
			&vecMemory[i]);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void SkyboxShaderUniforms::Cleanup(VulkanDevice* pDevice)
{
	for (uint16_t i = 0; i < vecBuffer.size(); ++i)
	{
		vkDestroyBuffer(pDevice->m_vkLogicalDevice, vecBuffer[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecMemory[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void SkyboxShaderUniforms::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}

//---------------------------------------------------------------------------------------------------------------------
Skybox::Skybox()
{
	m_vecVertices.clear();
	m_vecIndices.clear();
	
	m_pShaderUniformsMVP	= nullptr;
	m_pCubemap				= nullptr;
	
	m_vkVertexBuffer		= VK_NULL_HANDLE;
	m_vkIndexBuffer			= VK_NULL_HANDLE;
	m_vkVertexBufferMemory	= VK_NULL_HANDLE;
	m_vkIndexBufferMemory	= VK_NULL_HANDLE;
	
	m_vkDescriptorPool		= VK_NULL_HANDLE;
	m_vkDescriptorSetLayout = VK_NULL_HANDLE;
	m_vecDescriptorSet.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Skybox::~Skybox()
{
	SAFE_DELETE(m_pShaderUniformsMVP);
	SAFE_DELETE(m_pCubemap);
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::CreateSkybox(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Create Vertex data for skybox
	m_vecVertices.reserve(8);
	m_vecVertices.emplace_back(glm::vec3(-1, -1,  1));
	m_vecVertices.emplace_back(glm::vec3( 1, -1,  1));
	m_vecVertices.emplace_back(glm::vec3( 1,  1,  1));
	m_vecVertices.emplace_back(glm::vec3(-1,  1,  1));
	m_vecVertices.emplace_back(glm::vec3(-1, -1, -1));
	m_vecVertices.emplace_back(glm::vec3( 1, -1, -1));
	m_vecVertices.emplace_back(glm::vec3( 1,  1, -1));
	m_vecVertices.emplace_back(glm::vec3(-1,  1, -1));
	
	CreateVertexBuffer(pDevice);

	// Create Index data for skybox
	m_vecIndices.reserve(36);
	m_vecIndices.emplace_back(0);	m_vecIndices.emplace_back(1);	m_vecIndices.emplace_back(2);
	m_vecIndices.emplace_back(2);	m_vecIndices.emplace_back(3);	m_vecIndices.emplace_back(0);
	
	m_vecIndices.emplace_back(3);	m_vecIndices.emplace_back(2);	m_vecIndices.emplace_back(6);
	m_vecIndices.emplace_back(6);	m_vecIndices.emplace_back(7);	m_vecIndices.emplace_back(3);
	
	m_vecIndices.emplace_back(7);	m_vecIndices.emplace_back(6);	m_vecIndices.emplace_back(5);
	m_vecIndices.emplace_back(5);	m_vecIndices.emplace_back(4);	m_vecIndices.emplace_back(7);

	m_vecIndices.emplace_back(4);	m_vecIndices.emplace_back(5);	m_vecIndices.emplace_back(1);
	m_vecIndices.emplace_back(1);	m_vecIndices.emplace_back(0);	m_vecIndices.emplace_back(4);

	m_vecIndices.emplace_back(4);	m_vecIndices.emplace_back(0);	m_vecIndices.emplace_back(3);
	m_vecIndices.emplace_back(3);	m_vecIndices.emplace_back(7);	m_vecIndices.emplace_back(4);

	m_vecIndices.emplace_back(1);	m_vecIndices.emplace_back(5);	m_vecIndices.emplace_back(6);
	m_vecIndices.emplace_back(6);	m_vecIndices.emplace_back(2);	m_vecIndices.emplace_back(1);

	CreateIndexBuffer(pDevice);

	// Load Cubemap!
	m_pCubemap = new VulkanTextureCUBE();
	m_pCubemap->CreateTextureCUBE(pDevice, "Yokohama2");

	// Setup descriptors!
	SetupDescriptors(pDevice, pSwapchain);
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::UpdateUniformBuffers(VulkanDevice* pDevice, uint32_t index)
{
	// Copy View Projection data
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, m_pShaderUniformsMVP->vecMemory[index], 0, sizeof(SkyboxShaderData), 0, &data);
	memcpy(data, &m_pShaderUniformsMVP->shaderData, sizeof(SkyboxShaderData));
	vkUnmapMemory(pDevice->m_vkLogicalDevice, m_pShaderUniformsMVP->vecMemory[index]);
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update Model matrix!
	m_pShaderUniformsMVP->shaderData.model = glm::mat4(1);

	// Fetch View & Projection matrices from the Camera!	
	m_pShaderUniformsMVP->shaderData.projection = FreeCamera::getInstance().m_matProjection;
	
	m_pShaderUniformsMVP->shaderData.view = glm::mat4(glm::mat3(FreeCamera::getInstance().m_matView));
	//m_pShaderUniformsMVP->shaderData.projection[1][1] *= -1.0f;

	// Update object ID
	m_pShaderUniformsMVP->shaderData.objectID = 0;
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::Render(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index)
{
	VkDeviceSize offsets[] = { 0 };
	
	vkCmdBindVertexBuffers(pDevice->m_vecCommandBufferGraphics[index], 0, 1, &m_vkVertexBuffer, offsets);	

		// bind mesh index buffer, with zero offset & using uint32_t type
	vkCmdBindIndexBuffer(pDevice->m_vecCommandBufferGraphics[index], m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	// bind descriptor sets
	vkCmdBindDescriptorSets(pDevice->m_vecCommandBufferGraphics[index],
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pPipeline->m_vkPipelineLayout,
							0,
							1,
							&(m_vecDescriptorSet[index]),
							0,
							nullptr);

	// Execute pipeline
	vkCmdDrawIndexed(pDevice->m_vecCommandBufferGraphics[index], 36, 1, 0, 0, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	m_pShaderUniformsMVP = new SkyboxShaderUniforms();
	m_pShaderUniformsMVP->CreateBuffers(pDevice, pSwapchain);

	// *** Create Descriptor pool
	std::array<VkDescriptorPoolSize, 2> arrDescriptorPoolSize = {};

	//-- Uniform Buffer
	arrDescriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	arrDescriptorPoolSize[0].descriptorCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());

	// Cubemap sampler
	arrDescriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorPoolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(arrDescriptorPoolSize.size());
	poolCreateInfo.pPoolSizes = arrDescriptorPoolSize.data();

	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Skybox Sampler Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Skybox Descriptor Pool");

	// *** Create Descriptor Set Layout
	std::array<VkDescriptorSetLayoutBinding, 2> arrDescriptorSetLayoutBindings = {};

	//-- Uniform Buffer
	arrDescriptorSetLayoutBindings[0].binding = 0;																// binding point in shader, binding = ?
	arrDescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;						// type of descriptor (uniform, dynamic uniform etc.) 
	arrDescriptorSetLayoutBindings[0].descriptorCount = 1;														// number of descriptors
	arrDescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;	// Shader stage to bind to
	arrDescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;												// For textures!

	//-- Cubemap Texture
	arrDescriptorSetLayoutBindings[1].binding = 1;
	arrDescriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	arrDescriptorSetLayoutBindings[1].descriptorCount = 1;
	arrDescriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	arrDescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descSetlayoutCreateInfo = {};
	descSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetlayoutCreateInfo.bindingCount = arrDescriptorSetLayoutBindings.size();
	descSetlayoutCreateInfo.pBindings = arrDescriptorSetLayoutBindings.data();
	
	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &descSetlayoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a Skybox Descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Skybox Descriptor set layout");

	// *** Create Descriptor Set per swapchain image!
	m_vecDescriptorSet.resize(pSwapchain->m_vecSwapchainImages.size());

	// we create copies of DescriptorSetLayout per swapchain image
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(pSwapchain->m_vecSwapchainImages.size(), m_vkDescriptorSetLayout);

	// Descriptor set allocation info 
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkDescriptorPool;													// pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());	// number of sets to allocate
	setAllocInfo.pSetLayouts = descriptorSetLayouts.data();												// layouts to use to allocate sets

	if (vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, m_vecDescriptorSet.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocated Skybox Descriptor sets");
	}
	else
		LOG_DEBUG("Successfully created Skybox Descriptor sets");


	// *** Update all the descriptor set bindings
	for (uint16_t i = 0; i < pSwapchain->m_vecSwapchainImages.size(); i++)
	{
		//-- Uniform Buffer
		VkDescriptorBufferInfo ubBufferInfo = {};
		ubBufferInfo.buffer = m_pShaderUniformsMVP->vecBuffer[i];			// buffer to get data from
		ubBufferInfo.offset = 0;											// position of start of data
		ubBufferInfo.range = sizeof(SkyboxShaderUniforms);					// size of data

		// Data about connection between binding & buffer
		VkWriteDescriptorSet ubSetWrite = {};
		ubSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubSetWrite.dstSet = m_vecDescriptorSet[i];							// Descriptor set to update
		ubSetWrite.dstBinding = 0;											// binding to update
		ubSetWrite.dstArrayElement = 0;										// index in array to update
		ubSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// type of descriptor
		ubSetWrite.descriptorCount = 1;										// amount to update		
		ubSetWrite.pBufferInfo = &ubBufferInfo;

		//-- Cubemap Texture
		VkDescriptorImageInfo cubemapImageInfo = {};
		cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;											
		cubemapImageInfo.imageView = m_pCubemap->m_vkTextureImageView;	
		cubemapImageInfo.sampler = m_pCubemap->m_vkTextureSampler;		

		// Descriptor write info
		VkWriteDescriptorSet cubemapSetWrite = {};
		cubemapSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cubemapSetWrite.dstSet = m_vecDescriptorSet[i];
		cubemapSetWrite.dstBinding = 1;
		cubemapSetWrite.dstArrayElement = 0;
		cubemapSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		cubemapSetWrite.descriptorCount = 1;
		cubemapSetWrite.pImageInfo = &cubemapImageInfo;

		// List of Descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { ubSetWrite, cubemapSetWrite };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::Cleanup(VulkanDevice* pDevice)
{
	m_pCubemap->Cleanup(pDevice);
	m_pShaderUniformsMVP->Cleanup(pDevice);

	vkDestroyBuffer(pDevice->m_vkLogicalDevice, m_vkVertexBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkVertexBufferMemory, nullptr);

	vkDestroyBuffer(pDevice->m_vkLogicalDevice, m_vkIndexBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, m_vkIndexBufferMemory, nullptr);

	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, m_vkDescriptorSetLayout, nullptr);

}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::CleanupOnWindowResize(VulkanDevice* pDevice)
{
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::CreateVertexBuffer(VulkanDevice* pDevice)
{
	// Get the size of buffer needed for vertices
	VkDeviceSize bufferSize = 8 * sizeof(Helper::App::VertexP);

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create buffer & allocate memory to it!
	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							&stagingBuffer,
							&stagingBufferMemory);

	//-- MAP MEMORY TO VERTEX BUFFER
	void* data;																					// 1. Create pointer to a point in normal memory
	vkMapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);		// 2. Map the vertex buffer memory to that point
	memcpy(data, m_vecVertices.data(), (size_t)bufferSize);										// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);								// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER_BIT)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU & accessible by it & not CPU!
	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							&m_vkVertexBuffer,
							&m_vkVertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU using Command buffer!
	pDevice->CopyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

	// Clean up staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void Skybox::CreateIndexBuffer(VulkanDevice* pDevice)
{
	// Get size of buffer needed for indices
	VkDeviceSize bufferSize = 36 * sizeof(uint32_t);

	// Temporary buffer to "stage" index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							&stagingBuffer,
							&stagingBufferMemory);

	// Map memory to Index buffer
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_vecIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory);

	// Create buffer for index data on GPU access only area
	pDevice->CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							&m_vkIndexBuffer,
							&m_vkIndexBufferMemory);

	// Copy from staging buffer to GPU access buffer
	pDevice->CopyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

	// Clean up staging buffers
	vkDestroyBuffer(pDevice->m_vkLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(pDevice->m_vkLogicalDevice, stagingBufferMemory, nullptr);
}
