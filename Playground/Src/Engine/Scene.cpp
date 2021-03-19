#include "PlaygroundPCH.h"
#include "Scene.h"

#include "Renderer/VulkanDevice.h"
#include "Renderer/VulkanSwapChain.h"
#include "Renderer/VulkanGraphicsPipeline.h"

#include "Engine/RenderObjects/Skybox.h"
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
	Model* pModelPunk = new Model(ModelType::STATIC_OPAQUE);
	pModelPunk->LoadModel(pDevice, "Models/Sphere.fbx");
	pModelPunk->SetPosition(glm::vec3(0, 2, 0));
	pModelPunk->SetScale(glm::vec3(0.75f));
	pModelPunk->SetupDescriptors(pDevice, pSwapchain);

	m_vecModels.push_back(pModelPunk);

	// Load WoodenFloor Model
	Model* pWoodenFloor = new Model(ModelType::STATIC_OPAQUE);
	pWoodenFloor->LoadModel(pDevice, "Models/Plane_Oak.fbx");
	pWoodenFloor->SetPosition(glm::vec3(0, -2, 0));
	pWoodenFloor->SetScale(glm::vec3(4));
	pWoodenFloor->SetupDescriptors(pDevice, pSwapchain);

	m_vecModels.push_back(pWoodenFloor);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Update(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, float dt)
{
	// Update each model's uniform data
	for (Model* element : m_vecModels)
	{
		if (element != nullptr)
		{
			element->Update(pDevice, pSwapchain, dt);
		}
	}

	// Update Skybox data!
	Skybox::getInstance().Update(pDevice, pSwapchain, dt);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::UpdateUniforms(VulkanDevice* pDevice, uint32_t imageIndex)
{
	// Update all models!
	for (Model* element :m_vecModels)
	{
		if (element != nullptr)
		{
			element->UpdateUniformBuffers(pDevice, imageIndex);
		}
	}

	// Update Skybox uniforms!
	Skybox::getInstance().UpdateUniformBuffers(pDevice, imageIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::RenderOpaque(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex)
{
	// Draw Scene!
	for (Model* element : m_vecModels)
	{
		if (element != nullptr)
		{
			element->Render(pDevice, pPipline, imageIndex);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::RenderSkybox(VulkanDevice* pDevice, VulkanGraphicsPipeline* pPipline, uint32_t imageIndex)
{
	Skybox::getInstance().Render(pDevice, pPipline, imageIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void Scene::Cleanup(VulkanDevice* pDevice)
{
	for (Model* element : m_vecModels)
	{
		element->Cleanup(pDevice);
	}	
}



