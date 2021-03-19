#pragma once

#include "vulkan/vulkan.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Engine/Helpers/Utility.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanTextureCUBE;
class VulkanGraphicsPipeline;

//---------------------------------------------------------------------------------------------------------------------
struct SkyboxShaderData
{
	SkyboxShaderData()
	{
		model = glm::mat4(1);
		view = glm::mat4(1);
		projection = glm::mat4(1);
		objectID = 0;
	}

	// Data Specific
	glm::mat4							model;
	glm::mat4							view;
	glm::mat4							projection;
	uint32_t							objectID;
};
//---------------------------------------------------------------------------------------------------------------------
struct SkyboxShaderUniforms
{
	SkyboxShaderUniforms()
	{
		vecBuffer.clear();
		vecMemory.clear();
	}

	void								CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

	SkyboxShaderData					shaderData;

	// Vulkan Specific
	std::vector<VkBuffer>				vecBuffer;
	std::vector<VkDeviceMemory>			vecMemory;
};

//---------------------------------------------------------------------------------------------------------------------
class Skybox
{
public:
	static Skybox& getInstance()
	{
		static Skybox instance;
		return instance;
	}
	
	~Skybox();

	void								CreateSkybox(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								UpdateUniformBuffers(VulkanDevice* pDevice, uint32_t index);
	void								Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void								Render(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index);
	void								SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	Skybox();
	void								CreateVertexBuffer(VulkanDevice* pDevice);
	void								CreateIndexBuffer(VulkanDevice* pDevice);

public:
	VulkanTextureCUBE*					m_pCubemap;
	VkDescriptorPool					m_vkDescriptorPool;					// Pool for all descriptors.
	VkDescriptorSetLayout				m_vkDescriptorSetLayout;			// combination of layouts of uniforms & samplers.
	std::vector<VkDescriptorSet>		m_vecDescriptorSet;					// combination of sets of uniforms & samplers per swapchain image!

private:
	std::vector<Helper::App::VertexP>	m_vecVertices;
	std::vector<uint32_t>				m_vecIndices;
	SkyboxShaderUniforms*				m_pShaderUniformsMVP;
	VkBuffer							m_vkVertexBuffer;
	VkBuffer							m_vkIndexBuffer;
	VkDeviceMemory						m_vkVertexBufferMemory;
	VkDeviceMemory						m_vkIndexBufferMemory;
};

