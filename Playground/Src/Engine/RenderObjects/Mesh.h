#pragma once

#include "PlaygroundPCH.h"
#include "vulkan/vulkan.h"

#include "Engine/Helpers/Utility.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct PushConstantData
{
	glm::mat4 matModel;
};

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice physicalDevice,
		VkDevice logicalDevice,
		VkQueue transferQueue,
		VkCommandPool transferCommandPool,
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

	PushConstantData			m_pushConstData;

	int							m_iTexID;

	VkPhysicalDevice			m_vkPhysicalDevice;
	VkDevice					m_vkDevice;

	uint32_t					m_uiVertexCount;
	VkBuffer					m_vkVertexBuffer;
	VkDeviceMemory				m_vkVertexBufferMemory;

	uint32_t					m_uiIndexCount;
	VkBuffer					m_vkIndexBuffer;
	VkDeviceMemory				m_vkIndexBufferMemory;

	void						CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, const std::vector<Helper::App::VertexPCT>& vertices);
	void						CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, const std::vector<uint32_t>& indices);
};

