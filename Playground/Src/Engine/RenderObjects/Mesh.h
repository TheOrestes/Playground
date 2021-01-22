#pragma once

#include "PlaygroundPCH.h"
#include "vulkan/vulkan.h"

#include "Engine/Helpers/Utility.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

class VulkanDevice;

struct PushConstantData
{
	glm::mat4 matModel;
};

class Mesh
{
public:
	Mesh();
	Mesh(const VulkanDevice* device,
		VkDevice logicalDevice,
		const std::vector<Helper::App::VertexPCT>& vertices,
		const std::vector<uint32_t>& indices,
		int texID);

	void						SetPushConstantData(glm::mat4 modelMatrix);
	inline PushConstantData		GetPushConstantData() { return m_pushConstData; }

	inline uint32_t				getVertexCount() const { return m_uiVertexCount; }
	inline VkBuffer				getVertexBuffer() const { return m_vkVertexBuffer; }

	inline uint32_t				getIndexCount() const { return m_uiIndexCount; }
	inline VkBuffer				getIndexBuffer() const { return m_vkIndexBuffer; }

	inline int					getTexID() const { return m_iTexID; }

	~Mesh();

	void						Cleanup();

private:

	VkPhysicalDevice			m_vkPhysicalDevice;
	VkDevice					m_vkLogicalDevice;

	PushConstantData			m_pushConstData;

	int							m_iTexID;

	VkDevice					m_vkDevice;

	uint32_t					m_uiVertexCount;
	VkBuffer					m_vkVertexBuffer;
	VkDeviceMemory				m_vkVertexBufferMemory;

	uint32_t					m_uiIndexCount;
	VkBuffer					m_vkIndexBuffer;
	VkDeviceMemory				m_vkIndexBufferMemory;

	void						CreateVertexBuffer(const VulkanDevice* device, const std::vector<Helper::App::VertexPCT>& vertices);
	void						CreateIndexBuffer(const VulkanDevice* device, const std::vector<uint32_t>& indices);
};

