#pragma once

#include "PlaygroundPCH.h"

#include "Mesh.h"
#include "assimp/scene.h"

class Model
{
public:
	Model() {};
	Model(std::vector<Mesh> newMeshList);
	~Model() {};

	static std::vector<std::string>		LoadMaterials(const aiScene* scene);
	static std::vector<Mesh>			LoadNode(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferPool,
												 aiNode* node, const aiScene* scene, std::vector<int> vecMatToTexture);
	static Mesh							LoadMesh(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkQueue transferQueue, VkCommandPool transferPool,
												 aiMesh* mesh, const aiScene* scene, std::vector<int> vecMatToTexture);

	uint64_t							GetMeshCount() const;
	Mesh*								GetMesh(uint64_t index);

	glm::mat4							GetModelMatrix() const;
	void								SetModelMatrix(glm::mat4 newModel);

	void								DestroyModel();


private:
	std::vector<Mesh>					m_vecMeshes;
	glm::mat4							m_matModel;
};

