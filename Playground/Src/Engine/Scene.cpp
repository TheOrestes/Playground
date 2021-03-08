#include "PlaygroundPCH.h"
#include "Scene.h"

#include "Renderer/VulkanDevice.h"
#include "Renderer/VulkanSwapChain.h"
#include "Engine/RenderObjects/Model.h"

//---------------------------------------------------------------------------------------------------------------------
Scene::Scene()
{
	m_vecModels.clear();
}

//---------------------------------------------------------------------------------------------------------------------
Scene::~Scene()
{
	m_vecModels.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::LoadScene(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Load all 3D models...
	LoadModels(pDevice, pSwapchain);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::LoadModels(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	// Load SteamPunk Model
	Model* pModelPunk = new Model();
	pModelPunk->LoadModel(pDevice, "Models/SteamPunk.fbx");
	pModelPunk->SetPosition(glm::vec3(0, 0, 0));
	pModelPunk->SetScale(glm::vec3(0.75f));
	pModelPunk->SetupDescriptors(pDevice, pSwapchain);

	m_vecModels.push_back(pModelPunk);

	// Load WoodenFloor Model
	Model* pWoodenFloor = new Model();
	pWoodenFloor->LoadModel(pDevice, "Models/Plane_Oak.fbx");
	pWoodenFloor->SetPosition(glm::vec3(0, -2, 0));
	pWoodenFloor->SetScale(glm::vec3(4));
	pWoodenFloor->SetupDescriptors(pDevice, pSwapchain);

	m_vecModels.push_back(pWoodenFloor);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Cleanup(VulkanDevice* pDevice)
{
	for (Model* element : m_vecModels)
	{
		element->Cleanup(pDevice);
	}	
}


