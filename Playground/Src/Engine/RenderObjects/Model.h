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
		vecBuffer.clear();
		vecMemory.clear();
	}

	void								CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

	ShaderData							shaderData;

	// Vulkan Specific
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

	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

	void								SetPosition(const glm::vec3& _pos);
	void								SetRotation(const glm::vec3& _axis, float angle);
	void								SetScale(const glm::vec3& _scale);

private:
	std::vector<Mesh>					LoadNode(VulkanDevice* device, aiNode* node, const aiScene* scene);

	void								LoadTextureFromMaterial(aiMaterial* pMaterial, aiTextureType eType);
	void								LoadMaterials(VulkanDevice* device, const aiScene* scene);
	Mesh								LoadMesh(VulkanDevice* device, aiMesh* mesh, const aiScene* scene);

private:
	std::vector<Mesh>					m_vecMeshes;
	std::map<std::string, TextureType>	m_mapTextures;

	VulkanMaterial*						m_pMaterial;
	ShaderUniforms*						m_pShaderUniformsMVP;

public:
	VkDescriptorPool					m_vkDescriptorPool;					// Pool for all descriptors.
	VkDescriptorSetLayout				m_vkDescriptorSetLayout;			// combination of layouts of uniforms & samplers.
	std::vector<VkDescriptorSet>		m_vecDescriptorSet;					// combination of sets of uniforms & samplers per swapchain image!

private:
	glm::vec3							m_vecPosition;
	glm::vec3							m_vecRotationAxis;
	float								m_fAngle;
	glm::vec3							m_vecScale;
};

