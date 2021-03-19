#pragma once

class VulkanDevice;
class VulkanSwapChain;
class VulkanGraphicsPipeline;
class Model;

class Scene
{
public:
	Scene();
	~Scene();
	
	void						LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void						Cleanup(VulkanDevice* pDevice);

	void						Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt);
	void						UpdateUniforms(VulkanDevice* pDevice, uint32_t imageIndex);
	void						RenderOpaque(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex);
	void						RenderSkybox(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex);

	inline std::vector<Model*>	GetModelList() { return m_vecModels; }

private:
	void						LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

private:
	std::vector<Model*>			m_vecModels;
};

