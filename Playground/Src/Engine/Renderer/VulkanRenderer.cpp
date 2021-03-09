
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "VulkanRenderer.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "DeferredFrameBuffer.h"
#include "VulkanMaterial.h"
#include "VulkanTexture.h"
#include "VulkanGraphicsPipeline.h"

#include "Engine/Scene.h"
#include "Engine/RenderObjects/Model.h"
#include "Engine/Helpers/Utility.h"
#include "Engine/ImGui/UIManager.h"
#include "Engine/ImGui/imgui.h"
#include "Engine/ImGui/imgui_impl_glfw.h"
#include "Engine/ImGui/imgui_impl_vulkan.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanRenderer::VulkanRenderer()
{
	m_pWindow							= nullptr;

	m_pDevice							= nullptr;
	m_pSwapChain						= nullptr;
	m_pFrameBuffer						= nullptr;
	m_pScene							= nullptr;

	m_pGraphicsPipelineOpaque			= nullptr;
	m_pGraphicsPipelineDeferred			= nullptr;
	
	m_uiCurrentFrame					= 0;
	m_bFramebufferResized				= false;

	m_vkInstance						= VK_NULL_HANDLE;
	m_vkDebugMessenger					= VK_NULL_HANDLE;
	m_vkSurface							= VK_NULL_HANDLE;

	m_vkRenderPass						= VK_NULL_HANDLE;

	m_vkDeferredPassDescriptorSetLayout = VK_NULL_HANDLE;
	
	m_vecSemaphoreImageAvailable.clear();
	m_vecSemaphoreRenderFinished.clear();
	m_vecFencesRender.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanRenderer::~VulkanRenderer()
{
	m_vecSemaphoreImageAvailable.clear();
	m_vecSemaphoreRenderFinished.clear();
	m_vecFencesRender.clear();

	SAFE_DELETE(m_pScene);
	SAFE_DELETE(m_pGraphicsPipelineOpaque);
	SAFE_DELETE(m_pGraphicsPipelineDeferred);
	SAFE_DELETE(m_pFrameBuffer);
	SAFE_DELETE(m_pSwapChain);
	SAFE_DELETE(m_pDevice);
}

//---------------------------------------------------------------------------------------------------------------------
int VulkanRenderer::Initialize(GLFWwindow* pWindow)
{
	m_pWindow = pWindow;

	// register a callback to detect window resize
	glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);

	// Run shader compiler before everything else
	RunShaderCompiler("Shaders");

	try
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();

		m_pDevice = new VulkanDevice(m_vkInstance, m_vkSurface);

		m_pDevice->PickPhysicalDevice();
		m_pDevice->CreateLogicalDevice();
		
		m_pSwapChain = new VulkanSwapChain();
		m_pSwapChain->CreateSwapChain(m_pDevice, m_vkSurface, m_pWindow);

		m_pFrameBuffer = new DeferredFrameBuffer();
		m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_ALBEDO);
		m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_DEPTH);
		m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_NORMAL);
		m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_POSITION);

		CreateRenderPass();

		m_pFrameBuffer->CreateFrameBuffers(m_pDevice, m_pSwapChain, m_vkRenderPass);

		// Command pool & Command buffer for Graphics!
		m_pDevice->CreateGraphicsCommandPool();
		m_pDevice->CreateGraphicsCommandBuffers(m_pSwapChain->m_vecSwapchainImages.size());

		// Command pool & Command buffer for UI! 
		//m_pDevice->CreateGUICommandPool();
		//m_pDevice->CreateGUICommandBuffers(m_pSwapChain->m_vecSwapchainImages.size());
		m_pScene = new Scene();
		m_pScene->LoadScene(m_pDevice, m_pSwapChain);
		
		CreateGraphicsPipeline();

		//AllocateDynamicBufferTransferSpace();

		CreateDeferredPassDescriptorPool();
		CreateDeferredPassDescriptorSets();

		CreateSyncObjects();

		// Initialize UI Manager!
		UIManager::getInstance().Initialize(m_pWindow, m_vkInstance, m_pDevice, m_pSwapChain);
	}
	catch (const std::runtime_error& e)
	{
		std::cout << "\nERROR: " << e.what();
		return EXIT_FAILURE;
	}

	return 0;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::Update(float dt)
{
	for (Model* element : m_pScene->GetModelList())
	{
		if(element != nullptr)
		{
			element->Update(m_pDevice, m_pSwapChain, dt);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::RunShaderCompiler(const std::string& directoryPath)
{
	std::string shaderCompiler = "C:/VulkanSDK/1.2.162.1/Bin/glslc.exe";
	for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
	{
		if (entry.is_regular_file() && (entry.path().extension().string() == ".vert" || entry.path().extension().string() == ".frag"))
		{
			std::string cmd = shaderCompiler + " -c" + " " + entry.path().string() + " -o " + entry.path().string() + ".spv";
			LOG_DEBUG("Compiling shader " + entry.path().filename().string());
			std::system(cmd.c_str());
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateInstance()
{
	// Check if requested validation layer is available with current vulkan instance!
	if (Helper::Vulkan::g_bEnableValidationLayer && !CheckValidationLayerSupport())
	{
		LOG_ERROR("Requested Validation Layer not supported!");
	}

	// Provide information about our application, this struct is optional!
	VkApplicationInfo appInfo = {};
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "Hello Vulkan Triangle";
	appInfo.pEngineName = "Vulkan Engine";
	appInfo.pNext = nullptr;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	// Non-optional struct. This tells vulkan driver which global extensions
	// and validation layers we want to use. 
	VkInstanceCreateInfo createInfo{};

	uint32_t glfwExtensionCount = 0;

	//This function returns an array of names of Vulkan instance extensions required
	// by GLFW for creating Vulkan surfaces for GLFW windows.
	const char** extensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// const char** to std::vector<const char*> conversion 
	// Add debug messenger extension conditionally!
	std::vector<const char*> vecExtensions(extensions, extensions + glfwExtensionCount);
	if (Helper::Vulkan::g_bEnableValidationLayer)
	{
		vecExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Check if all required Instance Extensions are supported!
	if (!CheckInstanceExtensionSupport(vecExtensions))
	{
		LOG_ERROR("VkInstance does not support required extensions!");
	}
	else
	{
		LOG_INFO("VkInstance supports required extensions!");
	}

	// Create additional debug messenger just for vkCreateInstance & vkDestroyInstance!
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (Helper::Vulkan::g_bEnableValidationLayer)
	{
		createInfo.enabledLayerCount = Helper::Vulkan::g_strValidationLayers.size();
		createInfo.ppEnabledLayerNames = Helper::Vulkan::g_strValidationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	createInfo.enabledExtensionCount = vecExtensions.size();
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledExtensionNames = vecExtensions.data();
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
		LOG_ERROR("Failed to create Vulkan Instance!");

	LOG_INFO("Vulkan Instance Created!");
}

//---------------------------------------------------------------------------------------------------------------------
bool VulkanRenderer::CheckInstanceExtensionSupport(const std::vector<const char*>& instanceExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> vecExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vecExtensions.data());

#if defined _DEBUG
	// Enumerate all the extensions supported by the vulkan instance.
	// Ideally, this list should contain extensions requested by GLFW and
	// few addional ones!
	LOG_DEBUG("--------- Available Vulkan Extensions ---------");
	for (int i = 0; i < extensionCount; ++i)
	{
		LOG_DEBUG(vecExtensions[i].extensionName);
	}
	LOG_DEBUG("-----------------------------------------------");
#endif

	// Check if given extensions are in the list of available extensions
	for (uint32_t i = 0; i < extensionCount; i++)
	{
		bool hasExtension = false;

		for (uint32_t j = 0; j < instanceExtensions.size(); j++)
		{
			if (strcmp(vecExtensions[i].extensionName, instanceExtensions[j]))
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
			return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool VulkanRenderer::CheckValidationLayerSupport()
{
	// Enumerate available validation layers for the vulkan instance!
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> vecAvailableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, vecAvailableLayers.data());

	// Try to see if requested enumeration layers [in Helper.h] is present in available 
	// validation layers. 
	bool layerFound = false;
	for (int i = 0; i < Helper::Vulkan::g_strValidationLayers.size(); ++i)
	{
		for (int j = 0; j < layerCount; ++j)
		{
			if (strcmp(Helper::Vulkan::g_strValidationLayers[i], vecAvailableLayers[j].layerName) == 0)
			{
				layerFound = true;

				std::string msg = std::string(Helper::Vulkan::g_strValidationLayers[i]) + " validation layer found!";
				LOG_DEBUG(msg.c_str());
			}
		}
	}

	return layerFound;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.flags = 0;
	createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateSurface()
{
	if (glfwCreateWindowSurface(m_vkInstance, m_pWindow, nullptr, &m_vkSurface) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Windows surface!");
		return;
	}

	LOG_INFO("Windows surface created");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::HandleWindowResize()
{
	// Handle window minimization => framebuffer size = 0
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_pWindow, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_pWindow, &width, &height);
		glfwWaitEvents();
	}

	// we shouldn't touch resources that are still in use!
	vkDeviceWaitIdle(m_pDevice->m_vkLogicalDevice);

	// perform cleanup on old versions
	CleanupOnWindowResize();

	// Recreate...!
	LOG_DEBUG("Recreating SwapChain Start");

	m_pSwapChain->CreateSwapChain(m_pDevice, m_vkSurface, m_pWindow);

	m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_ALBEDO);
	m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_DEPTH);
	m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_NORMAL);
	m_pFrameBuffer->CreateAttachment(m_pDevice, m_pSwapChain, AttachmentType::FB_ATTACHMENT_POSITION);

	CreateRenderPass();
	CreateGraphicsPipeline();

	m_pFrameBuffer->CreateFrameBuffers(m_pDevice, m_pSwapChain, m_vkRenderPass);

	CreateDeferredPassDescriptorPool();
	CreateDeferredPassDescriptorSets();

	m_pDevice->CreateGraphicsCommandBuffers(m_pSwapChain->m_vecSwapchainImages.size());

	UIManager::getInstance().HandleWindowResize(m_pWindow, m_vkInstance, m_pDevice, m_pSwapChain);

	LOG_DEBUG("Recreating SwapChain End");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateGraphicsPipeline()
{
	//----- Create GBUFFER_OPAQUE Graphics pipeline!
	m_pGraphicsPipelineOpaque = new VulkanGraphicsPipeline(PipelineType::GBUFFER_OPAQUE, m_pSwapChain);

	std::vector<VkDescriptorSetLayout> setLayouts = { m_pScene->GetModelList().at(0)->m_vkDescriptorSetLayout };
	
	VkPushConstantRange pushConstantRange = {};

	m_pGraphicsPipelineOpaque->CreatePipelineLayout(m_pDevice, setLayouts, pushConstantRange);
	m_pGraphicsPipelineOpaque->CreateGraphicsPipeline(m_pDevice, m_pSwapChain, m_vkRenderPass, 0);

	
	//----- Create GBUFFER_BEAUTY Graphics pipeline!
	m_pGraphicsPipelineDeferred = new VulkanGraphicsPipeline(PipelineType::FINAL_BEAUTY, m_pSwapChain);

	//-- Create Descriptor Set Layout! 
	std::vector<VkDescriptorSetLayoutBinding> inputLayoutBinding;

	// Color input binding 
	VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
	colorInputLayoutBinding.binding = 0;
	colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputLayoutBinding.descriptorCount = 1;
	colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Depth input binding
	VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
	depthInputLayoutBinding.binding = 1;
	depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputLayoutBinding.descriptorCount = 1;
	depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Normal Input binding
	VkDescriptorSetLayoutBinding normalInputLayoutBinding = {};
	normalInputLayoutBinding.binding = 2;
	normalInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	normalInputLayoutBinding.descriptorCount = 1;
	normalInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Position Input binding
	VkDescriptorSetLayoutBinding positionInputLayoutBinding = {};
	positionInputLayoutBinding.binding = 3;
	positionInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	positionInputLayoutBinding.descriptorCount = 1;
	positionInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	inputLayoutBinding.push_back(colorInputLayoutBinding);
	inputLayoutBinding.push_back(depthInputLayoutBinding);
	inputLayoutBinding.push_back(normalInputLayoutBinding);
	inputLayoutBinding.push_back(positionInputLayoutBinding);

	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount = inputLayoutBinding.size();
	inputLayoutCreateInfo.pBindings = inputLayoutBinding.data();

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(m_pDevice->m_vkLogicalDevice, &inputLayoutCreateInfo, nullptr, &m_vkDeferredPassDescriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a Input Image Descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Input Image Descriptor set layout");

	std::vector<VkDescriptorSetLayout> deferredSetLayouts = { m_vkDeferredPassDescriptorSetLayout };

	m_pGraphicsPipelineDeferred->CreatePipelineLayout(m_pDevice, deferredSetLayouts, pushConstantRange);
	m_pGraphicsPipelineDeferred->CreateGraphicsPipeline(m_pDevice, m_pSwapChain, m_vkRenderPass, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateRenderPass()
{
	// Array of subpasses
	std::array<VkSubpassDescription, 2> subpasses = {};

	// *** SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)
	// Color Attachment (Input)
	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format				= m_pFrameBuffer->m_pAlbedoAttachment->attachmentFormat;		// format to use for attachment
	colorAttachmentDesc.samples				= VK_SAMPLE_COUNT_1_BIT;										// number of samples for multi-sampling
	colorAttachmentDesc.loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR;									// describes what to do with attachment before rendering
	colorAttachmentDesc.storeOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;								// describes what to do with attachment after rendering
	colorAttachmentDesc.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;								// describes what to do with stencil before rendering
	colorAttachmentDesc.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;								// describes what to do with stencil after rendering
	colorAttachmentDesc.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;									// image data layout before render pass starts
	colorAttachmentDesc.finalLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;						// image data layout after render pass

	// Depth attachment (Input)
	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format				= m_pFrameBuffer->m_pDepthAttachment->attachmentFormat;
	depthAttachmentDesc.samples				= VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Normal Attachment (Input)
	VkAttachmentDescription normalAttachmentDesc = {};
	normalAttachmentDesc.format				= m_pFrameBuffer->m_pNormalAttachment->attachmentFormat;
	normalAttachmentDesc.samples			= VK_SAMPLE_COUNT_1_BIT;
	normalAttachmentDesc.loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAttachmentDesc.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachmentDesc.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAttachmentDesc.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	normalAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Position attachment (Input)
	VkAttachmentDescription positionAttachmentDesc = {};
	positionAttachmentDesc.format			= m_pFrameBuffer->m_pPositionAttachment->attachmentFormat;
	positionAttachmentDesc.samples			= VK_SAMPLE_COUNT_1_BIT;
	positionAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	positionAttachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	positionAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	positionAttachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	positionAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	positionAttachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Color attachment (Input) Reference
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment			 = 1;
	colorAttachmentRef.layout				 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment (Input) Reference
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment			 = 2;
	depthAttachmentRef.layout				 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Normal attachment (Input) Reference
	VkAttachmentReference normalAttachmentRef = {};
	normalAttachmentRef.attachment			  = 3;
	normalAttachmentRef.layout				  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Position attachment (Input) Reference
	VkAttachmentReference positionAttachmentRef = {};
	positionAttachmentRef.attachment			= 4;
	positionAttachmentRef.layout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 3> attachmentRefs = { colorAttachmentRef, normalAttachmentRef, positionAttachmentRef };

	// Set up subpass 1 (Outputs 3 Color + 1 Depth attachment) 
	subpasses[0].pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount		= attachmentRefs.size();
	subpasses[0].pColorAttachments			= attachmentRefs.data();
	subpasses[0].pDepthStencilAttachment	= &depthAttachmentRef;


	//--- SUBPASS 2 ATTACHMENTS + REFERENCES
	// Swap chain Color Attachment 
	VkAttachmentDescription swapChainColorAttachmentDesc = {};
	swapChainColorAttachmentDesc.format			= m_pSwapChain->m_vkSwapchainImageFormat;		// format to use for attachment
	swapChainColorAttachmentDesc.samples		= VK_SAMPLE_COUNT_1_BIT;						// number of samples for multi sampling
	swapChainColorAttachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;					// describes what to do with attachment before rendering
	swapChainColorAttachmentDesc.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;					// describes what to do with attachment after rendering
	swapChainColorAttachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// describes what to do with stencil before rendering
	swapChainColorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// describes what to do with stencil after rendering
	swapChainColorAttachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;					// image data layout before render pass starts
	swapChainColorAttachmentDesc.finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;		//? changed from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR since UI gets drawn last! 					

	// Swap chain Color attachment Reference
	VkAttachmentReference swapChainColorAttachmentRef = {};
	swapChainColorAttachmentRef.attachment			  = 0;
	swapChainColorAttachmentRef.layout				  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// References to attachments that subpass will take input from
	std::array<VkAttachmentReference, 4> inputReferences;
	inputReferences[0].attachment	= 1;
	inputReferences[0].layout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[1].attachment	= 2;
	inputReferences[1].layout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[2].attachment	= 3;
	inputReferences[2].layout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[3].attachment	= 4;
	inputReferences[3].layout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Set up subpass 2 (Takes in 4 input attachments from subpass 1 & outputs one color output for final present!)
	subpasses[1].pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[1].colorAttachmentCount	= 1;												
	subpasses[1].pColorAttachments		= &swapChainColorAttachmentRef;
	subpasses[1].inputAttachmentCount	= static_cast<uint32_t>(inputReferences.size());
	subpasses[1].pInputAttachments		= inputReferences.data();

	// SUBPASS DEPENDENCIES
	// Need to determine when layout transition occurs using subpass dependencies
	std::array<VkSubpassDependency, 3> subpassDependencies;

	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL & VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	// Transition must happen after...
	subpassDependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;						// Sub pass index
	subpassDependencies[0].srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// pipeline stage
	subpassDependencies[0].srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;				// stage access mask (memory access)
	// But must happen before...
	subpassDependencies[0].dstSubpass		= 0;
	subpassDependencies[0].dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags	= 0;

	// Sub pass 1 layout (color+depth) to subpass 2 layout (shader read) i.e.
	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL & VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL to 
	// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	subpassDependencies[1].srcSubpass		= 0;
	subpassDependencies[1].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstSubpass		= 1;
	subpassDependencies[1].dstStageMask		= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].dstAccessMask	= VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags	= 0;

	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen after...
	subpassDependencies[2].srcSubpass		= 1;
	subpassDependencies[2].srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[2].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
	subpassDependencies[2].dstSubpass		= VK_SUBPASS_EXTERNAL;
	subpassDependencies[2].dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[2].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[2].dependencyFlags	= 0;

	// Render pass!
	std::array<VkAttachmentDescription, 5> renderPassAttachments = { swapChainColorAttachmentDesc, 
																	 colorAttachmentDesc, 
																	 depthAttachmentDesc, 
																	 normalAttachmentDesc, 
																	 positionAttachmentDesc };

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount	= static_cast<uint32_t>(renderPassAttachments.size());
	renderPassCreateInfo.pAttachments		= renderPassAttachments.data();
	renderPassCreateInfo.subpassCount		= static_cast<uint32_t>(subpasses.size());
	renderPassCreateInfo.pSubpasses			= subpasses.data();
	renderPassCreateInfo.dependencyCount	= static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies		= subpassDependencies.data();

	if (vkCreateRenderPass(m_pDevice->m_vkLogicalDevice, &renderPassCreateInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Render Pass");
	}
	else
		LOG_INFO("Created Render Pass!");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::RecordCommands(uint32_t currentImage)
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = 0;												// buffer can be re-submitted when it has already been submitted & is awaiting execution!

	// Information about how to begin a render pass (only needed for graphical applications) 
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkRenderPass;						// Render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };						// start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = m_pSwapChain->m_vkSwapchainExtent;			// size of region to run render pass on (starting at offset) 

	std::array<VkClearValue, 5> clearValues = {};

	clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValues[1].color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValues[2].depthStencil.depth = 1.0f;
	clearValues[3].color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValues[4].color = { 0.2f, 0.2f, 0.2f, 1.0f };

	renderPassBeginInfo.pClearValues = clearValues.data();								// list of clear values
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

	renderPassBeginInfo.framebuffer = m_pFrameBuffer->m_vecFramebuffers[currentImage];

	// start recording commands to command buffer
	if (vkBeginCommandBuffer(m_pDevice->m_vecCommandBufferGraphics[currentImage], &bufferBeginInfo) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to begin recording command buffer...");
	}
	else
	{
		// Begin Render Pass
		vkCmdBeginRenderPass(m_pDevice->m_vecCommandBufferGraphics[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind Pipeline to be used in render pass
		vkCmdBindPipeline(m_pDevice->m_vecCommandBufferGraphics[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineOpaque->m_vkGraphicsPipeline);
		
		// Draw Scene!
		for (Model* element : m_pScene->GetModelList())
		{
			if(element != nullptr)
			{
				element->Render(m_pDevice, m_pGraphicsPipelineOpaque, currentImage);
			}
		} 

		// Start second subpass
		vkCmdNextSubpass(m_pDevice->m_vecCommandBufferGraphics[currentImage], VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_pDevice->m_vecCommandBufferGraphics[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipelineDeferred->m_vkGraphicsPipeline);
		vkCmdBindDescriptorSets(m_pDevice->m_vecCommandBufferGraphics[currentImage],
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								m_pGraphicsPipelineDeferred->m_vkPipelineLayout,
								0, 1, &m_vecDeferredPassDescriptorSets[currentImage],
								0, nullptr);

		// Draw full screen triangle
		vkCmdDraw(m_pDevice->m_vecCommandBufferGraphics[currentImage], 3, 1, 0, 0);

		// End Render Pass
		vkCmdEndRenderPass(m_pDevice->m_vecCommandBufferGraphics[currentImage]);
	}
	

	// finish recording to command buffer
	if (vkEndCommandBuffer(m_pDevice->m_vecCommandBufferGraphics[currentImage]) != VK_SUCCESS)
		LOG_ERROR("Failed to record command buffer!");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateSyncObjects()
{
	m_vecSemaphoreImageAvailable.resize(Helper::App::MAX_FRAME_DRAWS);
	m_vecSemaphoreRenderFinished.resize(Helper::App::MAX_FRAME_DRAWS);
	m_vecFencesRender.resize(Helper::App::MAX_FRAME_DRAWS);

	// Semaphore create information
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.pNext = nullptr;

	// Fence create information
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceCreateInfo.pNext = nullptr;

	for (uint32_t i = 0; i < Helper::App::MAX_FRAME_DRAWS; ++i)
	{
		if (vkCreateSemaphore(m_pDevice->m_vkLogicalDevice, &semaphoreCreateInfo, nullptr, &m_vecSemaphoreImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_pDevice->m_vkLogicalDevice, &semaphoreCreateInfo, nullptr, &m_vecSemaphoreRenderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(m_pDevice->m_vkLogicalDevice, &fenceCreateInfo, nullptr, &m_vecFencesRender[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create Semaphores & fences!");
		}
		else
			LOG_INFO("Created Semaphores & fences!");
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateDeferredPassDescriptorPool()
{
	// INPUT ATTACHMENT DESCRIPTOR POOL
	// Color attachment 
	VkDescriptorPoolSize colorInputPoolSize = {};
	colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputPoolSize.descriptorCount = static_cast<uint32_t>(m_pSwapChain->m_vecSwapchainImages.size());
	
	// Depth attachment 
	VkDescriptorPoolSize depthInputPoolSize = {};
	depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputPoolSize.descriptorCount = static_cast<uint32_t>(m_pSwapChain->m_vecSwapchainImages.size());

	// Normal attachment
	VkDescriptorPoolSize normalInputPoolSize = {};
	normalInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	normalInputPoolSize.descriptorCount = static_cast<uint32_t>(m_pSwapChain->m_vecSwapchainImages.size());

	std::vector<VkDescriptorPoolSize> inputPoolSizes = { colorInputPoolSize, depthInputPoolSize, normalInputPoolSize };

	// Create input attachment pool
	VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
	inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	inputPoolCreateInfo.maxSets = m_pSwapChain->m_vecSwapchainImages.size();
	inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
	inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

	if (vkCreateDescriptorPool(m_pDevice->m_vkLogicalDevice, &inputPoolCreateInfo, nullptr, &m_vkDeferredPassDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Input Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Input Descriptor Pool");
}


//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CreateDeferredPassDescriptorSets()
{
	// Resize array to hold descriptor set for each swap chain image
	m_vecDeferredPassDescriptorSets.resize(m_pSwapChain->m_vecSwapchainImages.size());

	// Fill array of layouts ready for set creation
	std::vector<VkDescriptorSetLayout> setLayouts(m_pSwapChain->m_vecSwapchainImages.size(), m_vkDeferredPassDescriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkDeferredPassDescriptorPool;
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_pSwapChain->m_vecSwapchainImages.size());
	setAllocInfo.pSetLayouts = setLayouts.data();

	// Allocate Descriptor Sets
	VkResult result;
	result = vkAllocateDescriptorSets(m_pDevice->m_vkLogicalDevice, &setAllocInfo, m_vecDeferredPassDescriptorSets.data());

	if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocated Input Descriptor sets");
	}
	else
		LOG_DEBUG("Successfully created Input Descriptor sets");

	// Update each descriptor set with input attachment
	for (uint32_t i = 0; i < m_pSwapChain->m_vecSwapchainImages.size(); i++)
	{
		// color attachment descriptor
		VkDescriptorImageInfo colorAttachmentDescriptor = {};
		colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorAttachmentDescriptor.imageView = m_pFrameBuffer->m_pAlbedoAttachment->vecAttachmentImageView[i];
		colorAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Color attachment descriptor write
		VkWriteDescriptorSet colorWrite = {};
		colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colorWrite.dstSet = m_vecDeferredPassDescriptorSets[i];
		colorWrite.dstBinding = 0;
		colorWrite.dstArrayElement = 0;
		colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colorWrite.descriptorCount = 1;
		colorWrite.pImageInfo = &colorAttachmentDescriptor;

		// depth attachment descriptor
		VkDescriptorImageInfo depthAttachmentDescriptor = {};
		depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentDescriptor.imageView = m_pFrameBuffer->m_pDepthAttachment->vecAttachmentImageView[i];
		depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// depth attachment descriptor write
		VkWriteDescriptorSet depthWrite = {};
		depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		depthWrite.dstSet = m_vecDeferredPassDescriptorSets[i];
		depthWrite.dstBinding = 1;
		depthWrite.dstArrayElement = 0;
		depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthWrite.descriptorCount = 1;
		depthWrite.pImageInfo = &depthAttachmentDescriptor;

		// Normal attachment descriptor
		VkDescriptorImageInfo normalAttachmentDescriptor = {};
		normalAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalAttachmentDescriptor.imageView = m_pFrameBuffer->m_pNormalAttachment->vecAttachmentImageView[i];
		normalAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Normal attachment descriptor write
		VkWriteDescriptorSet normalWrite = {};
		normalWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		normalWrite.dstSet = m_vecDeferredPassDescriptorSets[i];
		normalWrite.dstBinding = 2;
		normalWrite.dstArrayElement = 0;
		normalWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		normalWrite.descriptorCount = 1;
		normalWrite.pImageInfo = &normalAttachmentDescriptor;

		// Position attachment descriptor
		VkDescriptorImageInfo positionAttachmentDescriptor = {};
		positionAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		positionAttachmentDescriptor.imageView = m_pFrameBuffer->m_pPositionAttachment->vecAttachmentImageView[i];
		positionAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Normal attachment descriptor write
		VkWriteDescriptorSet positionWrite = {};
		positionWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		positionWrite.dstSet = m_vecDeferredPassDescriptorSets[i];
		positionWrite.dstBinding = 3;
		positionWrite.dstArrayElement = 0;
		positionWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		positionWrite.descriptorCount = 1;
		positionWrite.pImageInfo = &positionAttachmentDescriptor;

		// List of input descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { colorWrite, depthWrite, normalWrite, positionWrite };

		// Update descriptor sets
		vkUpdateDescriptorSets(m_pDevice->m_vkLogicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::SetupDebugMessenger()
{
	if (!Helper::Vulkan::g_bEnableValidationLayer)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to setup debug messenger!");
		return;
	}
}

//---------------------------------------------------------------------------------------------------------------------
// 1. Acquire the next available image from the swap chain to draw. Set something to signal when we're finished with the image (semaphore)
// 2. Submit the command buffer to queue for execution, making sure it waits for the image to be signaled as available before drawing & signals when it has finished rendering!
// 3. Present image to the screen when it has signaled finished rendering!
void VulkanRenderer::Render()
{
	// 1. Acquire next image from the swap chain!
	// Wait for given fence to signal (open) from last draw call before continuing...
	vkWaitForFences(m_pDevice->m_vkLogicalDevice, 1, &m_vecFencesRender[m_uiCurrentFrame], VK_TRUE, UINT64_MAX);

	// Manually reset (close) fence!
	vkResetFences(m_pDevice->m_vkLogicalDevice, 1, &m_vecFencesRender[m_uiCurrentFrame]);

	// Get index of next image to be drawn to & signal semaphore when ready to be drawn to
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_pDevice->m_vkLogicalDevice, m_pSwapChain->m_vkSwapchain, UINT64_MAX, m_vecSemaphoreImageAvailable[m_uiCurrentFrame], VK_NULL_HANDLE, &imageIndex);

	// Record Graphics command
	RecordCommands(imageIndex);

	// Update all models!
	for (Model* element : m_pScene->GetModelList())
	{
		if(element != nullptr)
		{
			element->UpdateUniformBuffers(m_pDevice, imageIndex);
		}
	}
	

	UIManager::getInstance().BeginRender();
	UIManager::getInstance().RenderSceneUI(m_pScene);
	UIManager::getInstance().RenderDebugStats();
	UIManager::getInstance().EndRender(m_pSwapChain, imageIndex);

	// During any event such as window size change etc. we need to check if swap chain recreation is necessary
	// Vulkan tells us that swap chain in no longer adequate during presentation
	// VK_ERROR_OUT_OF_DATE_KHR = swap chain has become incompatible with the surface & can no longer be used for rendering. (window resize)
	// VK_SUBOPTIMAL_KHR = swap chain can be still used to present to the surface but the surface properties are no longer matching!

	// if swap chain is out of date while acquiring the image, then its not possible to present it!
	// We should recreate the swap chain & try again in the next draw call...
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		HandleWindowResize();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		LOG_ERROR("Failed to acquire swap chain image!");
		return;
	}

	// 2. Execute the command buffer
	// Queue submission & synchronization is configured through VkSubmitInfo.

	VkSemaphore waitSemaphores[] = { m_vecSemaphoreImageAvailable[m_uiCurrentFrame] };
	VkSemaphore signalSemaphores[] = { m_vecSemaphoreRenderFinished[m_uiCurrentFrame] };

	std::array<VkCommandBuffer, 2> commandBuffers = {
														m_pDevice->m_vecCommandBufferGraphics[imageIndex],
														UIManager::getInstance().m_vecCommandBuffers[imageIndex]
													};

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;														// Number of semaphores to wait on
	submitInfo.pWaitSemaphores = waitSemaphores;											// List of semaphores to wait on

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;												// stages to check semaphores at

	submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());			// number of command buffers to submit
	submitInfo.pCommandBuffers = commandBuffers.data();										// command buffers to submit
	submitInfo.signalSemaphoreCount = 1;													// number of semaphores to signal
	submitInfo.pSignalSemaphores = signalSemaphores;										// semaphores to signal when command buffer finishes
	submitInfo.pNext = nullptr;

	if (vkQueueSubmit(m_pDevice->m_vkQueueGraphics, 1, &submitInfo, m_vecFencesRender[m_uiCurrentFrame]) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to submit draw command buffer!");
	}

	//3. Submit result back to the swap chain.
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;														// Number of semaphores to wait on
	presentInfo.pWaitSemaphores = signalSemaphores;											// semaphores to wait on
	presentInfo.swapchainCount = 1;															// number of swap chains to present to
	presentInfo.pSwapchains = &(m_pSwapChain->m_vkSwapchain);								// swapchain to present image to
	presentInfo.pImageIndices = &imageIndex;												// index of images in swap chains to present
	presentInfo.pNext = nullptr;
	presentInfo.pResults = nullptr;

	// check if swap chain is optimal or not! else recreate & try in next draw call!
	result = vkQueuePresentKHR(m_pDevice->m_vkQueuePresent, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_bFramebufferResized)
	{
		m_bFramebufferResized = false;
		HandleWindowResize();
	}
	else if (result != VK_SUCCESS)
	{
		LOG_ERROR("Failed to present swap chain image!");
	}

	// Get next frame 
	m_uiCurrentFrame = (m_uiCurrentFrame + 1) % Helper::App::MAX_FRAME_DRAWS;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::AllocateDynamicBufferTransferSpace()
{
	// calculate alignment for model data!
	//m_uiModelUniformAlignment = (sizeof(UBOModel) + m_uiMinUniformBufferOffset - 1) & ~(m_uiMinUniformBufferOffset - 1);
	//
	//// Create space in memory to hold dynamic buffer that is aligned to our required alignment & holds MAX_OBJECTS!
	//m_pModelTransferSpace = (UBOModel*)_aligned_malloc(m_uiModelUniformAlignment * Helper::App::MAX_OBJECTS, m_uiModelUniformAlignment);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::CleanupOnWindowResize()
{
	UIManager::getInstance().CleanupOnWindowResize(m_pDevice);
	
	m_pGraphicsPipelineOpaque->CleanupOnWindowResize(m_pDevice);
	m_pGraphicsPipelineDeferred->CleanupOnWindowResize(m_pDevice);

	vkDestroyRenderPass(m_pDevice->m_vkLogicalDevice, m_vkRenderPass, nullptr);

	vkDestroyDescriptorPool(m_pDevice->m_vkLogicalDevice, m_vkDeferredPassDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_pDevice->m_vkLogicalDevice, m_vkDeferredPassDescriptorSetLayout, nullptr);

	m_pFrameBuffer->CleanupOnWindowResize(m_pDevice);
	m_pSwapChain->CleanupOnWindowResize(m_pDevice);
	m_pDevice->CleanupOnWindowResize();


	LOG_DEBUG("Old SwapChain Cleanup");
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanRenderer::Cleanup()
{
	// Wait until no action being run on device before destroying! 
	vkDeviceWaitIdle(m_pDevice->m_vkLogicalDevice);

	UIManager::getInstance().Cleanup(m_pDevice);
	
	m_pFrameBuffer->Cleanup(m_pDevice);

	m_pGraphicsPipelineDeferred->Cleanup(m_pDevice);
	m_pGraphicsPipelineOpaque->Cleanup(m_pDevice);

	vkDestroyRenderPass(m_pDevice->m_vkLogicalDevice, m_vkRenderPass, nullptr);

	vkDestroyDescriptorPool(m_pDevice->m_vkLogicalDevice, m_vkDeferredPassDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_pDevice->m_vkLogicalDevice, m_vkDeferredPassDescriptorSetLayout, nullptr);

	for (Model* element : m_pScene->GetModelList())
	{
		if(element != nullptr)
		{
			element->Cleanup(m_pDevice);
		}
	}
	
	// Destroy semaphores
	for (uint32_t i = 0; i < Helper::App::MAX_FRAME_DRAWS; ++i)
	{
		vkDestroySemaphore(m_pDevice->m_vkLogicalDevice, m_vecSemaphoreImageAvailable[i], nullptr);
		vkDestroySemaphore(m_pDevice->m_vkLogicalDevice, m_vecSemaphoreRenderFinished[i], nullptr);
		vkDestroyFence(m_pDevice->m_vkLogicalDevice, m_vecFencesRender[i], nullptr);
	}

	m_pSwapChain->Cleanup(m_pDevice);
	m_pDevice->Cleanup();

	if (Helper::Vulkan::g_bEnableValidationLayer)
	{
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}

