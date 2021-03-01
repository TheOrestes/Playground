#include "PlaygroundPCH.h"
#include "UIManager.h"

#include "Engine/Renderer/VulkanDevice.h"
#include "Engine/Renderer/VulkanSwapChain.h"
#include "Engine/Renderer/VulkanFrameBuffer.h"
#include "PlaygroundHeaders.h"
#include "Engine/Helpers/Log.h"
#include "Engine/Helpers/Utility.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

//---------------------------------------------------------------------------------------------------------------------
UIManager::UIManager()
{
	//m_pFramebuffer = nullptr; 
}

//---------------------------------------------------------------------------------------------------------------------
UIManager::~UIManager()
{
	//SAFE_DELETE(m_pFramebuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::Initialize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VkRenderPass renderPass)
{
	// Setup ImGui context!
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// setup ImGui style
	ImGui::StyleColorsDark();
	
	// Initialize Descriptor Pool
	InitDescriptorPool(pDevice);

	// Create Framebuffer
	//m_pFramebuffer = new VulkanFrameBuffer();
	//m_pFramebuffer->CreateAttachment(pDevice, pSwapchain);
	
	// Initialize Render Pass
	//InitRenderPass(pDevice, pSwapchain);

	//m_pFramebuffer->CreateFrameBuffers(pDevice, pSwapchain, m_vkRenderPass);

	// **** Initialize ImGui
	ImGui_ImplGlfw_InitForVulkan(pWindow, true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = instance;
	initInfo.PhysicalDevice = pDevice->m_vkPhysicalDevice;
	initInfo.Device = pDevice->m_vkLogicalDevice;
	initInfo.QueueFamily = pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value();
	initInfo.Queue = pDevice->m_vkQueueGraphics;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = m_vkDescriptorPool;
	initInfo.Allocator = nullptr;
	initInfo.MinImageCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());
	initInfo.ImageCount = static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size());
	initInfo.CheckVkResultFn = nullptr;
	ImGui_ImplVulkan_Init(&initInfo, renderPass);

	// Upload fonts to the GPU 
	VkCommandBuffer commandBuffer = pDevice->BeginCommandBuffer();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	pDevice->EndAndSubmitCommandBuffer(commandBuffer);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::InitDescriptorPool(VulkanDevice* pDevice)
{
	// Create Descriptor Pool
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	if (vkCreateDescriptorPool(pDevice->m_vkLogicalDevice, &pool_info, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create ImGui Descriptor Pool!");
	}
	else
		LOG_DEBUG("ImGui Descriptor Pool created!");
}

//---------------------------------------------------------------------------------------------------------------------
//void UIManager::InitRenderPass(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
//{
//	// **** First create attachment description
//	VkAttachmentDescription attachment = {};
//	attachment.format = m_pFramebuffer->m_attachmentFormat;						// format to use for attachment : same as framebuffer
//	attachment.samples = VK_SAMPLE_COUNT_1_BIT;									// number of samples for multi-sampling
//	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;								// we want to draw GUI over main rendering, hence we don't clear it. 
//	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// describes what to do with attachment after rendering
//	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// describes what to do with stencil before rendering
//	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// describes what to do with stencil after rendering
//	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		// image data layout before render pass starts
//	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// image data layout after render pass
//
//	// Actual reference to the attachment
//	VkAttachmentReference attachRef = {};
//	attachRef.attachment = 0;
//	attachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	// **** Create subpass for our Render pass
//	VkSubpassDescription subpass = {};
//	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subpass.colorAttachmentCount = 1;
//	subpass.pColorAttachments = &attachRef;
//
//	// Create subpass dependency 
//	VkSubpassDependency dependency = {};
//	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//	dependency.dstSubpass = 0;
//	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//
//	// **** Create Render Pass! 
//	VkRenderPassCreateInfo renderPassCreateInfo = {};
//	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassCreateInfo.attachmentCount = 1;
//	renderPassCreateInfo.pAttachments = &attachment;
//	renderPassCreateInfo.subpassCount = 1;
//	renderPassCreateInfo.pSubpasses = &subpass;
//	renderPassCreateInfo.dependencyCount = 1;
//	renderPassCreateInfo.pDependencies = &dependency;
//
//	if (vkCreateRenderPass(pDevice->m_vkLogicalDevice, &renderPassCreateInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
//	{
//		LOG_ERROR("ImGui Render pass creation failed!");
//	}
//	else
//		LOG_DEBUG("ImGui Render pass created!");
//}

//---------------------------------------------------------------------------------------------------------------------
//void UIManager::RecordCommands(VulkanDevice* pDevice, VulkanSwapChain* pSwapchain, VulkanFrameBuffer* pFrameBuffer, VkRenderPass renderPass, uint32_t currentImage)
//{	
//	// Information about how to begin each command buffer
//	VkCommandBufferBeginInfo bufferBeginInfo = {};
//	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//	bufferBeginInfo.flags = 0;
//
//	// Information about how to begin a render pass (only needed for graphical applications) 
//	VkRenderPassBeginInfo renderPassBeginInfo = {};
//	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//	renderPassBeginInfo.framebuffer = pFrameBuffer->m_vecFramebuffer[currentImage];
//	renderPassBeginInfo.renderPass = renderPass;									// Render pass to begin
//	renderPassBeginInfo.renderArea.offset = { 0,0 };									// start point of render pass in pixels
//	renderPassBeginInfo.renderArea.extent = pSwapchain->m_vkSwapchainExtent;			// size of region to run render pass on (starting at offset)
//
//	VkClearValue clearValue = {};
//	clearValue.color = { 0.2f, 0.2f, 0.2f, 1.0f };
//
//	renderPassBeginInfo.clearValueCount = 1;
//	renderPassBeginInfo.pClearValues = &clearValue;
//
//	if(vkBeginCommandBuffer(pDevice->m_vecCommandBufferGUI[currentImage], &bufferBeginInfo) != VK_SUCCESS)
//	{
//		LOG_ERROR("UIManager: Failed to begin recording to Command Buffer");
//	}
//	else
//	{
//		vkCmdBeginRenderPass(pDevice->m_vecCommandBufferGUI[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pDevice->m_vecCommandBufferGUI[currentImage]);
//		vkCmdEndRenderPass(pDevice->m_vecCommandBufferGUI[currentImage]);
//	}
//
//	if (vkEndCommandBuffer(pDevice->m_vecCommandBufferGUI[currentImage]) != VK_SUCCESS)
//		LOG_ERROR("UIManager: Failed to end recording to command buffer");
//}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::BeginRender()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::EndRender()
{
	ImGui::Render();
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::HandleWindowResize(GLFWwindow* pWindow, VkInstance instance, VulkanDevice* pDevice, VulkanSwapChain* pSwapchain)
{
	int width, height;
	glfwGetFramebufferSize(pWindow, &width, &height);
	if (width > 0 && height > 0)
	{
		ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size()));
		//ImGui_ImplVulkanH_CreateOrResizeWindow(	instance,
		//										pDevice->m_vkPhysicalDevice,
		//										pDevice->m_vkLogicalDevice,
		//										m_pMainWindowData,
		//										pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value(),
		//										nullptr,
		//										width,
		//										height,
		//										static_cast<uint32_t>(pSwapchain->m_vecSwapchainImages.size()));

		// m_pMainWindowData->FrameIndex = 0;
	}
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::Cleanup(VulkanDevice* pDevice)
{
	//m_pFramebuffer->Cleanup(pDevice);
	vkDestroyDescriptorPool(pDevice->m_vkLogicalDevice, m_vkDescriptorPool, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void UIManager::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	//m_pFramebuffer->CleanupOnWindowResize(pDevice);
}
