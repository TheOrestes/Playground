#pragma once

#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

class VulkanDevice;
class VulkanSwapChain;
class VulkanMaterial;
class VulkanTexture2D;
class VulkanGraphicsPipeline;
enum class TextureType;

//---------------------------------------------------------------------------------------------------------------------
enum class ModelType
{
	STATIC_OBJECT = 1
};

//---------------------------------------------------------------------------------------------------------------------
struct ShaderData
{
	ShaderData()
	{
		model = glm::mat4(1);
		view = glm::mat4(1);
		projection = glm::mat4(1);
		objectID = 1;
	}

	// Data Specific
	glm::mat4							model;
	glm::mat4							view;
	glm::mat4							projection;
	uint32_t							objectID;
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
	Model(ModelType typeID);
	~Model();

	std::vector<Mesh>					LoadModel(VulkanDevice* device, const std::string& filePath);
	void								UpdateUniformBuffers(VulkanDevice* pDevice, uint32_t index);
	void								Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void								Render(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index);
	void								SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

	// --- SETTERS!
	inline void							SetPosition(const glm::vec3& _pos)		{ m_vecPosition = _pos; }
	inline void							SetRotationAxis(const glm::vec3& _axis) { m_vecRotationAxis = _axis; }
	inline void							SetRotationAngle(float angle)			{ m_fAngle = angle; }
	inline void							SetScale(const glm::vec3& _scale)		{ m_vecScale = _scale; }

	// --- GETTERS!
	inline	glm::vec3					GetPosition()							{ return m_vecPosition; }
	inline	glm::vec3					GetRotationAxis()						{ return  m_vecRotationAxis; }
	inline  float						GetRotationAngle()						{ return m_fAngle; }
	inline	glm::vec3					GetScale()								{ return m_vecScale; }

private:
	std::vector<Mesh>					LoadNode(VulkanDevice* device, aiNode* node, const aiScene* scene);

	void								SetDefaultTexture(aiTextureType eType);
	void								ExtractTextureFromMaterial(aiMaterial* pMaterial, aiTextureType eType);
	void								LoadMaterials(VulkanDevice* device, const aiScene* scene);
	Mesh								LoadMesh(VulkanDevice* device, aiMesh* mesh, const aiScene* scene);

private:
	std::vector<Mesh>					m_vecMeshes;
	std::map<std::string, TextureType>	m_mapTextures;

	ModelType							m_eType;
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

public:
	float								m_fCurrentAngle;
	bool								m_bAutoRotate;
	float								m_fAutoRotateSpeed;
	std::string							m_strName;
};

