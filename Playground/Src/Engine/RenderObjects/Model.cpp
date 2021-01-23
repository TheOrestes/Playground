
#include "PlaygroundPCH.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Renderer/VulkanDevice.h"

#include "Model.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Model::Model(std::vector<Mesh> newMeshList)
{
	m_vecMeshes = newMeshList;
	m_matModel = glm::mat4(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> Model::LoadMaterials(const aiScene* scene)
{
	// create one to one sized list of textures
	std::vector<std::string> textureList(scene->mNumMaterials);

	// Go thorugh each material and copy its texture file name
	for (uint32_t i = 0; i < scene->mNumMaterials; i++)
	{
		// Get the material
		aiMaterial* material = scene->mMaterials[i];

		textureList[i] = "";

#ifdef _DEBUG
		int ambientTex = material->GetTextureCount(aiTextureType_AMBIENT);
		int baseTex = material->GetTextureCount(aiTextureType_BASE_COLOR);
		int diffuseTex = material->GetTextureCount(aiTextureType_DIFFUSE);
#endif

		// check for the diffuse texture
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// get the path of the texture file
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				// cut off any directory information already present
				int idx = std::string(path.data).rfind("\\");
				std::string fileName = std::string(path.data).substr(idx + 1);

				textureList[i] = fileName;
			}
		}
	}

	return textureList;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<Mesh> Model::LoadNode(const VulkanDevice* device, aiNode* node, const aiScene* scene, std::vector<int> vecMatToTexture)
{
	std::vector<Mesh> vecMesh;

	// Go through each mesh at this node & create it, then add it to our mesh list
	for (uint64_t i = 0; i < node->mNumMeshes; i++)
	{
		vecMesh.push_back(LoadMesh(device,
								scene->mMeshes[node->mMeshes[i]],
								scene,
								vecMatToTexture));
	}

	// Go through each node attached to this node & load it, then append their meshes to this node's mesh list
	for (uint64_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> newList = LoadNode(device, node->mChildren[i], scene, vecMatToTexture);
		vecMesh.insert(vecMesh.end(), newList.begin(), newList.end());
	}

	return vecMesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mesh Model::LoadMesh(const VulkanDevice* device, aiMesh* mesh, const aiScene* scene, std::vector<int> vecMatToTexture)
{
	std::vector<Helper::App::VertexPCT>  vertices;
	std::vector<uint32_t>				indices;

	vertices.resize(mesh->mNumVertices);

	for (uint64_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].Position = { mesh->mVertices[i].x, mesh->mVertices[i].y,  mesh->mVertices[i].z };

		// Set texture coords (if they exists)
		if (mesh->mTextureCoords[0])
		{
			vertices[i].UV = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].UV = { 0.0f, 0.0f };
		}

		// Set color (just use white for now)
		vertices[i].Color = { 1.0f, 1.0f, 1.0f };
	}

	// iterate over indices thorough faces & copy across
	for (uint64_t i = 0; i < mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = mesh->mFaces[i];

		// go thorugh face's indices & add to the list
		for (uint16_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create new mesh with details & return it!
	Mesh newMesh(device, vertices, indices, vecMatToTexture[mesh->mMaterialIndex]);
	return newMesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t Model::GetMeshCount() const
{
	return m_vecMeshes.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mesh* Model::GetMesh(uint64_t index)
{
	if (index >= m_vecMeshes.size())
		LOG_ERROR("Attempting to access Invalid Mesh Index!");

	return &m_vecMeshes[index];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
glm::mat4 Model::GetModelMatrix() const
{
	return m_matModel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Model::SetModelMatrix(glm::mat4 newModel)
{
	m_matModel = newModel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Model::DestroyModel()
{
	std::vector<Mesh>::iterator iter = m_vecMeshes.begin();

	for (; iter != m_vecMeshes.end(); iter++)
	{
		(*iter).Cleanup();
	}
}
