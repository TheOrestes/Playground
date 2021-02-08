#pragma once

#include "PlaygroundPCH.h"

#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Engine/Renderer/VulkanTexture.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanMaterial;
class VulkanGraphicsPipeline;

//---------------------------------------------------------------------------------------------------------------------
struct ShaderData
{
	ShaderData()
	{
		model = glm::mat4(1);
		view = glm::mat4(1);
		projection = glm::mat4(1);
	}

	// Data Specific
	glm::mat4							model;
	glm::mat4							view;
	glm::mat4							projection;
};
//---------------------------------------------------------------------------------------------------------------------
struct ShaderUniforms
{
	ShaderUniforms()
	{
		descriptorSetLayout = VK_NULL_HANDLE;
		descriptorPool = VK_NULL_HANDLE;

		vecDescriptorSets.clear();
		vecBuffer.clear();
		vecMemory.clear();
	}

	void								CreateDescriptorSetLayout(VulkanDevice* pDevice);
	void								CreateDescriptorPool(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								CreateDescriptorSet(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

	ShaderData							shaderData;

	// Vulkan Specific
	VkDescriptorSetLayout				descriptorSetLayout;
	VkDescriptorPool					descriptorPool;
	std::vector<VkDescriptorSet>		vecDescriptorSets;
	std::vector<VkBuffer>				vecBuffer;
	std::vector<VkDeviceMemory>			vecMemory;
};

//---------------------------------------------------------------------------------------------------------------------
class Model
{
public:
	Model();
	~Model();

	std::vector<Mesh>					LoadModel(VulkanDevice* device, const std::string& filePath);

	void								UpdateUniformBuffers(VulkanDevice* pDevice, uint32_t index);
	void								Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void								Render(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index);

	void								SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

	uint64_t							GetMeshCount() const;
	Mesh*								GetMesh(uint64_t index);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

private:
	std::vector<Mesh>					LoadNode(VulkanDevice* device, aiNode* node, const aiScene* scene);

	void								LoadMaterials(VulkanDevice* device, const aiScene* scene);
	Mesh								LoadMesh(VulkanDevice* device, aiMesh* mesh, const aiScene* scene);

private:
	std::vector<Mesh>					m_vecMeshes;
	std::map<std::string, TextureType>	m_mapTextures;

public:
	VulkanMaterial*						m_pMaterial;
	ShaderUniforms*						m_pShaderUniformsMVP;
};

