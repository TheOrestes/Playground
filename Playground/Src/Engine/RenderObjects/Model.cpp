
#include "PlaygroundPCH.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanMaterial.h"
#include "Engine/Renderer/VulkanGraphicsPipeline.h"

#include "Model.h"

//---------------------------------------------------------------------------------------------------------------------
Model::Model()
{
	m_vecMeshes.clear();

	m_pMaterial = nullptr;
	m_pShaderUniformsMVP = nullptr;
	
	m_mapTextures.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Model::~Model()
{
	m_mapTextures.clear();
	m_vecMeshes.clear();

	SAFE_DELETE(m_pShaderUniformsMVP);
	SAFE_DELETE(m_pMaterial);
}

//---------------------------------------------------------------------------------------------------------------------
std::vector<Mesh> Model::LoadModel(VulkanDevice* device, const std::string& filePath)
{
	// Import Model scene
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	if (!scene)
		LOG_CRITICAL("Failed to load model!");

	// Get list of textures based on materials!
	LoadMaterials(device, scene);

	return LoadNode(device, scene->mRootNode, scene);
}

//---------------------------------------------------------------------------------------------------------------------
void Model::UpdateUniformBuffers(VulkanDevice* pDevice, uint32_t index)
{
	// Copy View Projection data
	void* data;
	vkMapMemory(pDevice->m_vkLogicalDevice, m_pShaderUniformsMVP->vecMemory[index], 0, sizeof(ShaderData), 0, &data);
	memcpy(data, &m_pShaderUniformsMVP->shaderData, sizeof(ShaderData));
	vkUnmapMemory(pDevice->m_vkLogicalDevice, m_pShaderUniformsMVP->vecMemory[index]);
}

//---------------------------------------------------------------------------------------------------------------------
std::vector<Mesh> Model::LoadNode(VulkanDevice* device, aiNode* node, const aiScene* scene)
{
	// Go through each mesh at this node & create it, then add it to our mesh list
	for (uint64_t i = 0; i < node->mNumMeshes; i++)
	{
		m_vecMeshes.push_back(LoadMesh(device,
								scene->mMeshes[node->mMeshes[i]],
								scene));
	}

	// Go through each node attached to this node & load it, then append their meshes to this node's mesh list
	for (uint64_t i = 0; i < node->mNumChildren; i++)
	{
		LoadNode(device, node->mChildren[i], scene);
		//m_vecMeshes.insert(m_vecMeshes.end(), newList.begin(), newList.end());
	}

	return m_vecMeshes;
}

//---------------------------------------------------------------------------------------------------------------------
void Model::LoadMaterials(VulkanDevice* pDevice, const aiScene* scene)
{
	// Go through each material and copy its texture file name
	for (uint32_t i = 0; i < scene->mNumMaterials; i++)
	{
		// Get the material
		aiMaterial* material = scene->mMaterials[i];

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

				m_mapTextures.emplace(fileName, TextureType::TEXTURE_ALBEDO);
			}
		}
	}

	m_pMaterial = new VulkanMaterial();

	std::map<std::string, TextureType>::iterator iter = m_mapTextures.begin();
	for (; iter != m_mapTextures.end(); ++iter)
	{
		if (iter->first.empty())
		{
			// Load default texture (preferably bright pink to show missing texture!)
		}
		else
		{
			m_pMaterial->LoadTexture(pDevice, iter->first, iter->second);
		}
	}

	// Once we have loaded all the textures, create their descriptors!
	m_pMaterial->CreateDescriptorPool(pDevice);
	m_pMaterial->CreateDescriptorSetLayout(pDevice);
	m_pMaterial->CreateDescriptorSets(pDevice, 0);
}

//---------------------------------------------------------------------------------------------------------------------
Mesh Model::LoadMesh(VulkanDevice* pDevice, aiMesh* mesh, const aiScene* scene)
{
	std::vector<Helper::App::VertexPNTBT>	vertices;
	std::vector<uint32_t>					indices;
	std::vector<uint32_t>					textureIDs;

	vertices.resize(mesh->mNumVertices);

	// Loop through each vertex...
	for (uint64_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].Position = { mesh->mVertices[i].x, mesh->mVertices[i].y,  mesh->mVertices[i].z };

		// Set Normals
		vertices[i].Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

		if (mesh->mTangents || mesh->mBitangents)
		{
			vertices[i].Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			vertices[i].BiNormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
		}

		// Set texture coords (if they exists)
		if (mesh->mTextureCoords[0])
		{
			vertices[i].UV = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertices[i].UV = { 0.0f, 0.0f };
		}
	}

	// iterate over indices thorough faces for index data...
	for (uint64_t i = 0; i < mesh->mNumFaces; i++)
	{
		// Get a face
		aiFace face = mesh->mFaces[i];

		// go through face's indices & add to the list
		for (uint16_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create new mesh with details & return it!
	Mesh newMesh(pDevice, vertices, indices);
	return newMesh;
}

//---------------------------------------------------------------------------------------------------------------------
void Model::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update angle
	static float angle = 0.0f;
	angle += dt;
	if (angle > 360.0f) { angle = 0.0f; }

	// Update Model matrix!
	m_pShaderUniformsMVP->shaderData.model = glm::mat4(1);
	m_pShaderUniformsMVP->shaderData.model = glm::rotate(m_pShaderUniformsMVP->shaderData.model, angle, glm::vec3(0, 1, 0));

	// Fetch View & Projection matrices from the Camera!
	
	m_pShaderUniformsMVP->shaderData.projection = glm::perspective(	glm::radians(45.0f),
																	(float)pSwapchain->m_vkSwapchainExtent.width / (float)pSwapchain->m_vkSwapchainExtent.height,
																	0.1f,
																	1000.0f );

	m_pShaderUniformsMVP->shaderData.view = glm::lookAt(glm::vec3(10, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	m_pShaderUniformsMVP->shaderData.projection[1][1] *= -1.0f;
}

//---------------------------------------------------------------------------------------------------------------------
void Model::Render (VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipeline, uint32_t index)
{
	for (int i = 0; i < m_vecMeshes.size(); ++i)
	{

		VkBuffer vertexBuffers[] = { m_vecMeshes[i].m_vkVertexBuffer };										// Buffers to bind
		VkBuffer indexBuffer = m_vecMeshes[i].m_vkIndexBuffer;
		VkDeviceSize offsets[] = { 0 };																			// offsets into buffers being bound
		vkCmdBindVertexBuffers(pDevice->m_vecCommandBufferGraphics[index], 0, 1, vertexBuffers, offsets);		// Command to bind vertex buffer before drawing with them

		// bind mesh index buffer, with zero offset & using uint32_t type
		vkCmdBindIndexBuffer(pDevice->m_vecCommandBufferGraphics[index], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_pShaderUniformsMVP->vecDescriptorSets[index], m_pMaterial->m_vkDescriptorSet };

		// bind descriptor sets
		vkCmdBindDescriptorSets(pDevice->m_vecCommandBufferGraphics[index],
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								pPipeline->m_vkPipelineLayout,
								0,
								static_cast<uint32_t>(descriptorSetGroup.size()),
								descriptorSetGroup.data(),
								0,
								nullptr);

		// Execute pipeline
		vkCmdDrawIndexed(pDevice->m_vecCommandBufferGraphics[index], m_vecMeshes[i].m_uiIndexCount, 1, 0, 0, 0);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Model::SetupDescriptors(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	m_pShaderUniformsMVP = new ShaderUniforms();

	m_pShaderUniformsMVP->CreateDescriptorSetLayout(pDevice);
	m_pShaderUniformsMVP->CreateBuffers(pDevice, pSwapchain);
	m_pShaderUniformsMVP->CreateDescriptorPool(pDevice, pSwapchain);
	m_pShaderUniformsMVP->CreateDescriptorSet(pDevice, pSwapchain);
}

//---------------------------------------------------------------------------------------------------------------------
uint64_t Model::GetMeshCount() const
{
	return m_vecMeshes.size();
}

//---------------------------------------------------------------------------------------------------------------------
Mesh* Model::GetMesh(uint64_t index)
{
	if (index >= m_vecMeshes.size())
		LOG_ERROR("Attempting to access Invalid Mesh Index!");

	return &m_vecMeshes[index];
}

//---------------------------------------------------------------------------------------------------------------------
void Model::Cleanup(VulkanDevice* pDevice)
{
	m_pShaderUniformsMVP->Cleanup(pDevice);
	m_pMaterial->Cleanup(pDevice);

	std::vector<Mesh>::iterator iter = m_vecMeshes.begin();
	for (; iter != m_vecMeshes.end(); iter++)
	{
		(*iter).Cleanup(pDevice);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Model::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CreateDescriptorSetLayout(VulkanDevice* pDevice)
{
	// UNIFORM VALUE DESCRIPTOR SET LAYOUT
	// Shader Uniforms binding info
	VkDescriptorSetLayoutBinding uniformLayoutBinding = {};
	uniformLayoutBinding.binding = 0;																// binding point in shader
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;						// type of descriptor (uniform, dynamic uniform etc.) 
	uniformLayoutBinding.descriptorCount = 1;														// number of descriptors
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;									// Shader stage to bind to
	uniformLayoutBinding.pImmutableSamplers = nullptr;												// For textures!

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { uniformLayoutBinding };

	// Create descriptor set layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings = layoutBindings.data();

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(pDevice->m_vkLogicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a Uniform Descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Uniform Descriptor set layout");
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CreateDescriptorPool(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	//--- UNIFORM DESCRIPTOR POOL
	// type of descriptors & number of descriptors
	// ViewProjection Pool
	VkDescriptorPoolSize uniformPoolSize = {};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = static_cast<uint32_t>(vecBuffer.size());

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { uniformPoolSize };

	// data to create descriptor pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(pSwapChain->m_vecSwapchainImages.size());	// maximum number of descriptor sets that can be created from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());			// amount of pool sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();										// Pool sizes to create pool with

	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Descriptor Pool");
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CreateDescriptorSet(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	// Resize descriptor sets so one for every buffer
	vecDescriptorSets.resize(pSwapChain->m_vecSwapchainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(pSwapChain->m_vecSwapchainImages.size(), descriptorSetLayout);

	// Descriptor set allocation info 
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descriptorPool;											// pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(pSwapChain->m_vecSwapchainImages.size());	// number of sets to allocate
	setAllocInfo.pSetLayouts = setLayouts.data();														// layouts to use to allocate sets

	if (vkAllocateDescriptorSets(pDevice->m_vkLogicalDevice, &setAllocInfo, vecDescriptorSets.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocated Descriptor sets");
	}
	else
		LOG_DEBUG("Successfully created Descriptor sets");

	// Update all the descriptor set bindings
	for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		// VIEW PROJECTION DESCRIPTOR
		// buffer info & data offset info
		VkDescriptorBufferInfo vpbufferInfo = {};
		vpbufferInfo.buffer = vecBuffer[i];									// buffer to get data from
		vpbufferInfo.offset = 0;											// position of start of data
		vpbufferInfo.range = sizeof(ShaderUniforms);						// size of data

		// Data about connection between binding & buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = vecDescriptorSets[i];							// Descriptor set to update
		vpSetWrite.dstBinding = 0;											// binding to update
		vpSetWrite.dstArrayElement = 0;										// index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// type of descriptor
		vpSetWrite.descriptorCount = 1;										// amount to update		
		vpSetWrite.pBufferInfo = &vpbufferInfo;

		// List of Descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(pDevice->m_vkLogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CreateBuffers(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain)
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(ShaderUniforms);

	// one uniform buffer for each image (and by extension command buffer)
	vecBuffer.resize(pSwapChain->m_vecSwapchainImages.size());
	vecMemory.resize(pSwapChain->m_vecSwapchainImages.size());

	// create uniform buffers
	for (uint16_t i = 0; i < pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		pDevice->CreateBuffer(vpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vecBuffer[i],
			&vecMemory[i]);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::Cleanup(VulkanDevice* pDevice)
{
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pDevice->m_vkLogicalDevice, descriptorSetLayout, nullptr);

	for (uint16_t i = 0; i < vecBuffer.size(); ++i)
	{
		vkDestroyBuffer(pDevice->m_vkLogicalDevice, vecBuffer[i], nullptr);
		vkFreeMemory(pDevice->m_vkLogicalDevice, vecMemory[i], nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void ShaderUniforms::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}


