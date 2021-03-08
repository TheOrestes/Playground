#pragma once

class VulkanDevice;
class VulkanSwapChain;
class Model;

class Scene
{
public:
	Scene();
	~Scene();
	
	void						LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);
	void						Cleanup(VulkanDevice* pDevice);

	inline std::vector<Model*>	GetModelList() { return m_vecModels; }

private:
	void						LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain);

private:
	std::vector<Model*>			m_vecModels;
};

