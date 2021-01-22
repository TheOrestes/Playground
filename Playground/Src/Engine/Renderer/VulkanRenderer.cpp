
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "VulkanRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Engine/Helpers/Utility.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VulkanRenderer::VulkanRenderer()
{
	m_pWindow = nullptr;

	m_pDevice = nullptr;
	
	m_uiCurrentFrame = 0;
	m_bFramebufferResized = false;

	m_vkInstance = VK_NULL_HANDLE;
	m_vkDebugMessenger = VK_NULL_HANDLE;
	m_vkSurface = VK_NULL_HANDLE;

	m_vkDevice = VK_NULL_HANDLE;

	m_vkSwapchain = VK_NULL_HANDLE;
	m_vecSwapchainImages.clear();

	m_vkRenderPass = VK_NULL_HANDLE;
	m_vkPipelineLayout = VK_NULL_HANDLE;
	m_vkGraphicsPipeline = VK_NULL_HANDLE;

	m_vecFramebuffers.clear();
	m_vecSwapchainImageViews.clear();
	
	m_vecCommandBuffer.clear();

	m_vecSemaphoreImageAvailable.clear();
	m_vecSemaphoreRenderFinished.clear();
	m_vecFencesRender.clear();

	m_vecMeshes.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VulkanRenderer::~VulkanRenderer()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VulkanRenderer::Initialize(GLFWwindow* pWindow)
{
	m_pWindow = pWindow;

	// register a callback to detect window resize
	glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);

	try
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();

		m_pDevice = new VulkanDevice(m_vkInstance, m_vkSurface);
		m_pDevice->PickPhysicalDevice();
		
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateColorBufferImage();
		CreateDepthBufferImage();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreatePushConstantRange();
		CreateGraphicsPipeline();
		CreateFramebuffers();

		m_pDevice->CreateGraphicsCommandPool(m_vkDevice);
		m_pDevice->CreateGraphicsCommandBuffers(m_vecSwapchainImages.size(), m_vkDevice);

		m_vecCommandBuffer = m_pDevice->m_vecCommandBuffer;
		
		CreateTextureSampler();
		//AllocateDynamicBufferTransferSpace();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateInputDescriptorSets();
		CreateSyncObjects();


		// Create Projection matrix
		m_uboViewProjection.matProjection = glm::perspective(
			glm::radians(45.0f),
			(float)m_vkSwapchainExtent.width / (float)m_vkSwapchainExtent.height,
			0.1f,
			1000.0f
		);

		// Create view matrix
		m_uboViewProjection.matView = glm::lookAt(glm::vec3(10, 2, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		m_uboViewProjection.matProjection[1][1] *= -1.0f;

		//-- Create a mesh
		// Vertex data
		std::vector<Helper::App::VertexPCT> meshVertices1;
		meshVertices1.reserve(4);

		meshVertices1.emplace_back(glm::vec3(-0.4f, 0.4f, 0.0f), glm::vec3(1, 0, 0), glm::vec2(1, 1));		// 0
		meshVertices1.emplace_back(glm::vec3(-0.4f, -0.4f, 0.0f), glm::vec3(0, 1, 0), glm::vec2(1, 0));		// 1
		meshVertices1.emplace_back(glm::vec3(0.4f, -0.4f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0, 0));		// 2
		meshVertices1.emplace_back(glm::vec3(0.4f, 0.4f, 0.0f), glm::vec3(1, 1, 0), glm::vec2(0, 1));		// 3

		std::vector<Helper::App::VertexPCT> meshVertices2;
		meshVertices2.reserve(4);

		meshVertices2.emplace_back(glm::vec3(-0.25f, 0.6f, 0.0f), glm::vec3(0, 1, 0), glm::vec2(1, 1));		// 4
		meshVertices2.emplace_back(glm::vec3(-0.25f, -0.6f, 0.0f), glm::vec3(1, 0, 0), glm::vec2(1, 0));	// 5
		meshVertices2.emplace_back(glm::vec3(0.25f, -0.6f, 0.0f), glm::vec3(0, 0, 1), glm::vec2(0, 0));		// 6
		meshVertices2.emplace_back(glm::vec3(0.25f, 0.6f, 0.0f), glm::vec3(1, 1, 0), glm::vec2(0, 1));		// 7

		// Index data
		std::vector<uint32_t> meshIndices;
		meshIndices.reserve(6);
		meshIndices.emplace_back(0);	meshIndices.emplace_back(1);	meshIndices.emplace_back(2);
		meshIndices.emplace_back(2);	meshIndices.emplace_back(3);	meshIndices.emplace_back(0);

		int texID1 = CreateTexture("Randy.jpg");
		int texID2 = CreateTexture("Cartman.jpg");
		Mesh mesh1(m_pDevice, m_vkDevice, meshVertices1, meshIndices, texID1);
		Mesh mesh2(m_pDevice, m_vkDevice, meshVertices2, meshIndices, texID2);

		m_vecMeshes.push_back(mesh1);
		m_vecMeshes.push_back(mesh2);

		CreateModel("Models/car.fbx");
		glm::mat4 testMat = glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(0, 1, 0));

		m_vecModels[0].SetModelMatrix(testMat);
	}
	catch (const std::runtime_error& e)
	{
		std::cout << "\nERROR: " << e.what();
		return EXIT_FAILURE;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::Update(float dt)
{
	// Update angle
	static float angle = 0.0f;
	angle += 10.0f * dt;
	if (angle > 360.0f) { angle -= 360.0f; }

	glm::mat4 firstModelMatrix(1.0f);
	glm::mat4 secondModelMatrix(1.0f);

	glm::mat4 objectModelMatrix(1.0f);

	firstModelMatrix = glm::translate(firstModelMatrix, glm::vec3(0.0f, 0.0f, -6.0f));
	firstModelMatrix = glm::rotate(firstModelMatrix, glm::radians(angle), glm::vec3(0, 0, 1));

	secondModelMatrix = glm::translate(secondModelMatrix, glm::vec3(0.0f, 0.0f, -5.5f));
	secondModelMatrix = glm::rotate(secondModelMatrix, glm::radians(-angle * 10.0f), glm::vec3(0, 0, 1));

	objectModelMatrix = glm::rotate(objectModelMatrix, glm::radians(angle), glm::vec3(0, 1, 0));

	UpdateModel(0, firstModelMatrix);
	UpdateModel(1, secondModelMatrix);
	UpdateModel(-1, objectModelMatrix);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::UpdateModel(int modelID, glm::mat4 modelMatrix)
{
	if (modelID < m_vecMeshes.size())
		m_vecMeshes[modelID].SetPushConstantData(modelMatrix);

	// Updating model
	if (modelID < 0)
	{
		for (uint32_t i = 0; i < m_vecModels.size(); i++)
		{
			m_vecModels[i].SetModelMatrix(modelMatrix);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateSurface()
{
	if (glfwCreateWindowSurface(m_vkInstance, m_pWindow, nullptr, &m_vkSurface) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Windows surface!");
		return;
	}

	LOG_INFO("Windows surface created");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// std::set allows only One unique value for input values, no duplicate is allowed, so if both Graphics Queue family
	// and Presentation Queue family index is same then it will avoid the duplicates and assign only one queue index!
	std::set<uint32_t> uniqueQueueFamilies =
	{
		m_pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value(),
		m_pDevice->m_pQueueFamilyIndices->m_uiPresentFamily.value()
	};

	float queuePriority = 1.0f;

	for (uint32_t queueFamily = 0; queueFamily < uniqueQueueFamilies.size(); ++queueFamily)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.flags = 0;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Specify used device features...
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;		// Enabling anisotropy!

	// Create logical device...
	VkDeviceCreateInfo createInfo{};
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	// These are part of Vulkan Instance from 1.1 & deprecated as part of logical device!
	createInfo.enabledExtensionCount = Helper::Vulkan::g_strDeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = Helper::Vulkan::g_strDeviceExtensions.data();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	if (Helper::Vulkan::g_bEnableValidationLayer)
	{
		createInfo.enabledLayerCount = Helper::Vulkan::g_strValidationLayers.size();
		createInfo.ppEnabledLayerNames = Helper::Vulkan::g_strValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_pDevice->m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create logical vulkan device!");
		return;
	}

	// The queues are automatically created along with the logical device, but we don't 
	// have a handle to interface with them yet. Since we are only creating a single queue
	// from this family, we will use index 0
	vkGetDeviceQueue(m_vkDevice, m_pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value(), 0, &(m_pDevice->m_vkQueueGraphics));
	vkGetDeviceQueue(m_vkDevice, m_pDevice->m_pQueueFamilyIndices->m_uiPresentFamily.value(), 0, &(m_pDevice->m_vkQueuePresent));

	LOG_INFO("Logical Device Created!");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SwapChainSupportDetails VulkanRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails swapChainDetails;

	// Start with basic surface capabilities...
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &swapChainDetails.capabilities);

	// Now query supported surface formats...
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, swapChainDetails.formats.data());
	}

	// Finally, query supported presentation modes...
	uint32_t presentationModesCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentationModesCount, nullptr);

	if (presentationModesCount != 0)
	{
		swapChainDetails.presentModes.resize(presentationModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentationModesCount, swapChainDetails.presentModes.data());
	}

	return swapChainDetails;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// If restricted, search for optimal format
	for (const auto& format : availableFormats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	// If can't find optimal format, then just return first format
	return availableFormats[0];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	// out of all Mailbox allows triple buffering, so if available use it, else use FIFO mode.
	for (uint32_t i = 0; i < availablePresentModes.size(); ++i)
	{
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentModes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the 
	// resolution of the window that we're drawing to.The range of the possible resolutions is defined in the 
	// VkSurfaceCapabilitiesKHR structure.Vulkan tells us to match the resolution of the window by setting the 
	// width and height in the currentExtent member.However, some window managers do allow us to differ here 
	// and this is indicated by setting the width and height in currentExtent to a special value : the maximum 
	// value of uint32_t. In that case we'll pick the resolution that best matches the window within the 
	// minImageExtent and maxImageExtent bounds.
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		// To handle window resize properly, query current width-height of framebuffer, instead of global value!
		int width, height;
		glfwGetFramebufferSize(m_pWindow, &width, &height);

		//VkExtent2D actualExtent = { Helper::App::WIDTH, Helper::App::HEIGHT };
		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VkFormat VulkanRenderer::ChooseSupportedFormats(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through options & find the compatible one
	for (VkFormat format : formats)
	{
		// Get properties for given formats on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_pDevice->m_vkPhysicalDevice, format, &properties);

		// depending on tiling choice, need to check for different bit flag
		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}

		LOG_ERROR("Failed to find matching format!");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateSwapChain()
{
	// Get swap chain details so we can pick the best setting!
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_pDevice->m_vkPhysicalDevice);

	// 1. CHOOSE BEST SURFACE FORMAT
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);

	// 2. CHOOSE BEST PRESENTATION MODE
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);

	// 3. CHOOSE SWAPCHAIN IMAGE RESOLUTION
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	// decide how many images to have in the swap chain, it's good practice to have an extra count.
	// Also make sure it does not exceed maximum number of images 
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	// Create SwapChain object
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.clipped = VK_TRUE;									// don't care about the obscured pixels by other window
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// ignore alpha channel to blend with other windows
	createInfo.flags = 0;
	createInfo.imageArrayLayers = 1;								// Number of layers each image has, always 1, except for stereo app.
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	// use image as color attachment
	createInfo.minImageCount = imageCount;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	createInfo.pNext = nullptr;
	createInfo.presentMode = presentMode;
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;	// transform to image, say 90 degrees!
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_vkSurface;

	// Specify how to handle swap chain images that will be used across multiple queue families. That will be the case 
	// in our application if the graphics queue family is different from the presentation queue. We'll be drawing on 
	// the images in the swap chain from the graphics queue and then submitting them on the presentation queue. 
	// There are two ways to handle images that are accessed from multiple queues. 
	
	std::array<uint32_t, 2> queueFamilyIndices = { m_pDevice->m_pQueueFamilyIndices->m_uiGraphicsFamily.value(), m_pDevice->m_pQueueFamilyIndices->m_uiPresentFamily.value() };

	// check if Graphics & Presentation family share the same index or not!
	if (queueFamilyIndices[0] != queueFamilyIndices[1])
	{
		// Images can be used across multiple queue families without explicit ownership transfers.
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}
	else
	{
		// An image is owned by one queue family at a time and ownership must be explicitly transferred before using 
		// it in another queue family. This option offers the best performance.
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	// Finally, create the swap chain...
	if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_vkSwapchain) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Swap chain!");
		return;
	}

	LOG_INFO("Swapchain created!");

	// Retrieve handle to swapchain images...
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, nullptr);
	m_vecSwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, m_vecSwapchainImages.data());

	LOG_INFO("Swapchain images created!");

	// store format & extent for later usage...
	m_vkSwapchainImageFormat = surfaceFormat.format;
	m_vkSwapchainExtent = extent;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
	vkDeviceWaitIdle(m_vkDevice);

	// perform cleanup on old versions
	CleanupOnWindowResize();

	// Recreate...!
	LOG_DEBUG("Recreating SwapChain Start");

	CreateSwapChain();
	CreateImageViews();
	CreateColorBufferImage();
	CreateDepthBufferImage();
	CreateRenderPass();

	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateUniformBuffers();
	//CreateDescriptorPool();
	//CreateDescriptorSetLayout();
	//CreatePushConstantRange();
	//CreateDescriptorSets();
	//CreateInputDescriptorSets();
	//CreateCommandBuffers();

	m_pDevice->CreateGraphicsCommandBuffers(m_vecSwapchainImages.size(), m_vkDevice);

	LOG_DEBUG("Recreating SwapChain End");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateImageViews()
{
	m_vecSwapchainImageViews.resize(m_vecSwapchainImages.size());

	for (uint32_t i = 0; i < m_vecSwapchainImageViews.size(); ++i)
	{
		m_vecSwapchainImageViews[i] = Helper::Vulkan::CreateImageView(m_vkDevice,
			m_vecSwapchainImages[i],
			m_vkSwapchainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
	}

	LOG_INFO("Swapchain Imageviews created!");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateDescriptorSetLayout()
{
	// UNIFORM VALUE DESCRIPTOR SET LAYOUT
	// UBOViewProjection binding info
	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0;																// binding point in shader
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;							// type of descriptor (uniform, dynamic uniform etc.) 
	vpLayoutBinding.descriptorCount = 1;														// number of descriptors
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;									// Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr;												// For textures!

	// VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	// modelLayoutBinding.binding = 1;
	// modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// modelLayoutBinding.descriptorCount = 1;
	// modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	// modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

	// Create descriptor set layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings = layoutBindings.data();

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(m_vkDevice, &layoutCreateInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Uniform Value Descriptor set layout");		

	// TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
	// Texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	// Create a descriptor set layout with given bindings for texture
	VkDescriptorSetLayoutCreateInfo samplerLayoutCreateInfo = {};
	samplerLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	samplerLayoutCreateInfo.bindingCount = 1;
	samplerLayoutCreateInfo.pBindings = &samplerLayoutBinding;

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(m_vkDevice, &samplerLayoutCreateInfo, nullptr, &m_vkSamplerDescriptorSetLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a sampler descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Sampler Descriptor set layout");

	// INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
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

	// array of input attachment bindings
	std::vector<VkDescriptorSetLayoutBinding> inputBindings = { colorInputLayoutBinding, depthInputLayoutBinding };

	VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
	inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
	inputLayoutCreateInfo.pBindings = inputBindings.data();

	// Create descriptor set layout
	if (vkCreateDescriptorSetLayout(m_vkDevice, &inputLayoutCreateInfo, nullptr, &m_vkInputSeLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create a Input Image Descriptor set layout");
	}
	else
		LOG_DEBUG("Successfully created a Input Image Descriptor set layout");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreatePushConstantRange()
{
	// Define push constant values...
	m_vkPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;			// Shader stage push constant will go to
	m_vkPushConstantRange.offset = 0;										// Offset into given data to pass to push constant 
	m_vkPushConstantRange.size = sizeof(PushConstantData);					// Size of data being passed
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateGraphicsPipeline()
{
	// read shader byte code...
	std::vector<char> vertShaderCode = Helper::App::ReadShaderFile("Shaders/vs_shader.spv");
	std::vector<char> fragShaderCode = Helper::App::ReadShaderFile("Shaders/fs_shader.spv");

	// create shader modules...
	VkShaderModule vertShaderModule = Helper::Vulkan::CreateShaderModule(m_vkDevice, vertShaderCode);
	VkShaderModule fragShaderModule = Helper::Vulkan::CreateShaderModule(m_vkDevice, fragShaderCode);

	// to actually use shaders, we need to assign them to a specific pipeline stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// How the data for the single vertex (including info such as Position, color, texcoords etc.) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;															// can bind multiple stream of daya, this defines which one?
	bindingDescription.stride = sizeof(Helper::App::VertexPCT);								// size of single vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;								// How to move between data after each vertex
																							// VK_VERTEX_INPUT_RATE_VERTEX : move on to the next vertex
																							// VK_VERTEX_INPUT_RATE_INSTANCE: move on to a vertex of next instance.

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

	// Position attribute
	attributeDescriptions[0].binding = 0;													// which binding the data is at (should be same as above)
	attributeDescriptions[0].location = 0;													// location in shader where data will be read from
	attributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;					// format the data will take (also helps define size of the data)
	attributeDescriptions[0].offset = offsetof(Helper::App::VertexPCT, Position);			// where this attribute is defined in the data for a single vertex

	// Color attribute
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Helper::App::VertexPCT, Color);

	// Texture attribute
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Helper::App::VertexPCT, UV);

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();			// List of vertex attribute descriptions (data format & where to bind to - from)
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;						// List of vertex binding descriptions (data spacing/strides info) 
	vertexInputInfo.flags = 0;
	vertexInputInfo.pNext = nullptr;

	// Input assembly 
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;						// primitive type to single list
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.pNext = nullptr;

	// Viewport rect
	VkViewport viewportInfo{};
	viewportInfo.x = 0.0f;
	viewportInfo.y = 0.0f;
	viewportInfo.width = (float)m_vkSwapchainExtent.width;
	viewportInfo.height = (float)m_vkSwapchainExtent.height;
	viewportInfo.minDepth = 0.0f;
	viewportInfo.maxDepth = 1.0f;

	// Scissor rect 
	VkRect2D scissorRect = {};
	scissorRect.offset = { 0, 0 };															// offset to use region from
	scissorRect.extent = m_vkSwapchainExtent;												// extent to describe region to use, starting at offset!

	// combine viewport & scissor rect info to create viewport info!
	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.flags = 0;
	viewportStateInfo.pNext = nullptr;
	viewportStateInfo.pScissors = &scissorRect;
	viewportStateInfo.pViewports = &viewportInfo;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.viewportCount = 1;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;									// which face of a triangle to cull
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;										// change if fragments beyond near/far planes are clipped.
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;						// treat clockwise as front side
	rasterizerCreateInfo.lineWidth = 1.0f;													// how thick the line should be drawn
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;								// how to handle filling points between vertices
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;								// whether to discard data & skip rasterizer. Never creates fragments, only suitable for pipelines without framebuffers!
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;										// whether to add depth bias to fragments (good for stopping "shadow acne")
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerCreateInfo.flags = 0;
	rasterizerCreateInfo.pNext = nullptr;


	// Multi-sampling
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;									// Enable multisampling or not
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;					// number of samples per fragment
	multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;
	multisamplingCreateInfo.flags = 0;
	multisamplingCreateInfo.minSampleShading = 1.0f;
	multisamplingCreateInfo.pNext = nullptr;
	multisamplingCreateInfo.pSampleMask = nullptr;


	// Depth & Stencil testing 
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;										// Enable depth check to determine fragment write
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;										// Enable writing to depth buffer to replace old values
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;								// Comparison opearation that allows an overwrite (is in front)
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;								// Depth bounds test, does the depth value exist between two bounds!
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;									// Enable/Disable Stencil test

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;	// apply blending on all channels!	
	colorBlendAttachment.blendEnable = VK_FALSE;											// enable/disable blending
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;										// alternative to calculations is to use logical operations
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	colorBlendStateCreateInfo.flags = 0;
	colorBlendStateCreateInfo.pNext = nullptr;

	// Dynamic state (nullptr for now!)

	// Pipeline layout
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { m_vkDescriptorSetLayout, m_vkSamplerDescriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &m_vkPushConstantRange;

	// create pipeline layout
	if (vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Pipeline layout!");
	}
	else
		LOG_INFO("Created Pipeline layout");


	// Fianlly, Create Graphics Pipeline!!!
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	graphicsPipelineCreateInfo.layout = m_vkPipelineLayout;
	graphicsPipelineCreateInfo.renderPass = m_vkRenderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.pNext = nullptr;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.flags = 0;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;


	if (vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Graphics Pipeline!");
	}
	else
		LOG_INFO("Created Graphics Pipeline!");

	// Destroy shader modules...
	vkDestroyShaderModule(m_vkDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, fragShaderModule, nullptr);

	//---------- SECOND PASS PIPELINE 
	// Second pass shaders
	// read shader byte code...
	std::vector<char> secondVertShaderCode = Helper::App::ReadShaderFile("Shaders/vs_Second.spv");
	std::vector<char> secondFragShaderCode = Helper::App::ReadShaderFile("Shaders/fs_Second.spv");

	// create shader modules...
	VkShaderModule secondVertShaderModule = Helper::Vulkan::CreateShaderModule(m_vkDevice, secondVertShaderCode);
	VkShaderModule secondFragShaderModule = Helper::Vulkan::CreateShaderModule(m_vkDevice, secondFragShaderCode);

	// set new shaders
	vertShaderStageInfo.module = secondVertShaderModule;
	fragShaderStageInfo.module = secondFragShaderModule;

	VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// No vertex data for second pass
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// disable writing to depth buffer
	depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

	// Create new pipeline layout
	VkPipelineLayoutCreateInfo secondPipelineLayoutCreateInfo = {};
	secondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	secondPipelineLayoutCreateInfo.setLayoutCount = 1;
	secondPipelineLayoutCreateInfo.pSetLayouts = &m_vkInputSeLayout;
	secondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	secondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// create second pipeline layout
	if (vkCreatePipelineLayout(m_vkDevice, &secondPipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout2) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Second Pipeline layout!");
	}
	else
		LOG_INFO("Created Second Pipeline layout");

	graphicsPipelineCreateInfo.pStages = secondShaderStages;		// update second shader stage list
	graphicsPipelineCreateInfo.layout = m_vkPipelineLayout2;		// Change pipeline layout for input attachment descriptor sets
	graphicsPipelineCreateInfo.subpass = 1;							// Use second subpass

	// Create second pipeline
	if (vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_vkGraphicsPipeline2) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Second Graphics Pipeline!");
	}
	else
		LOG_INFO("Created Second Graphics Pipeline!");

	// Destroy second shader modules...
	vkDestroyShaderModule(m_vkDevice, secondVertShaderModule, nullptr);
	vkDestroyShaderModule(m_vkDevice, secondFragShaderModule, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateColorBufferImage()
{
	m_vecColorBufferImage.resize(m_vecSwapchainImages.size());
	m_vecColorBufferImageMemory.resize(m_vecSwapchainImages.size());
	m_vecColorBufferImageView.resize(m_vecSwapchainImages.size());

	std::vector<VkFormat> formats = { VK_FORMAT_R8G8B8A8_UNORM };

	// Get supported format for color attachment
	m_vkColorBufferFormat = ChooseSupportedFormats(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);

	for (uint16_t i = 0; i < m_vecSwapchainImages.size(); i++)
	{
		// Create color buffer image
		m_vecColorBufferImage[i] = Helper::Vulkan::CreateImage(m_pDevice->m_vkPhysicalDevice,
			m_vkDevice,
			m_vkSwapchainExtent.width,
			m_vkSwapchainExtent.height,
			m_vkColorBufferFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&m_vecColorBufferImageMemory[i]);

		// Create color buffer image view!
		m_vecColorBufferImageView[i] = Helper::Vulkan::CreateImageView(m_vkDevice,
			m_vecColorBufferImage[i],
			m_vkColorBufferFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateDepthBufferImage()
{
	m_vecDepthBufferImage.resize(m_vecSwapchainImages.size());
	m_vecDepthBufferImageMemory.resize(m_vecSwapchainImages.size());
	m_vecDepthBufferImageView.resize(m_vecSwapchainImages.size());

	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

	m_vkDepthBufferFormat = ChooseSupportedFormats(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	for (uint16_t i = 0; i < m_vecSwapchainImages.size(); i++)
	{
		// create depth buffer image
		m_vecDepthBufferImage[i] = Helper::Vulkan::CreateImage(m_pDevice->m_vkPhysicalDevice,
			m_vkDevice,
			m_vkSwapchainExtent.width,
			m_vkSwapchainExtent.height,
			m_vkDepthBufferFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&m_vecDepthBufferImageMemory[i]);

		// Create depth buffer image view!
		m_vecDepthBufferImageView[i] = Helper::Vulkan::CreateImageView(m_vkDevice,
			m_vecDepthBufferImage[i],
			m_vkDepthBufferFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateRenderPass()
{
	// Array of subpasses
	std::array<VkSubpassDescription, 2> subpasses = {};

	// ATTACHMENTS

	//--- SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)
	// Color Attachment (Input)
	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = m_vkColorBufferFormat;											// format to use for attachment
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;										// number of samples for multisampling
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;									// describes what to do with attachment before rendering
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;								// describes what to do with attachment after rendering
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;						// describes what to do with stencil before rendering
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						// describes what to do with stencil after rendering
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;								// image data layout before render pass starts
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;					// image data layout after render pass

	// Depth attachment (Input)
	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = m_vkDepthBufferFormat;
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Color attachment (Input) Reference
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 1;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment (Input) Reference
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 2;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Set up subpass 1
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount = 1;
	subpasses[0].pColorAttachments = &colorAttachmentRef;
	subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;


	//--- SUBPASS 2 ATTACHMENTS + REFERENCES
	// Swapchain Color Attachment 
	VkAttachmentDescription swapChainColorAttachmentDesc = {};
	swapChainColorAttachmentDesc.format = m_vkSwapchainImageFormat;								// format to use for attachment
	swapChainColorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;								// number of samples for multisampling
	swapChainColorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// describes what to do with attachment before rendering
	swapChainColorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// describes what to do with attachment after rendering
	swapChainColorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// describes what to do with stencil before rendering
	swapChainColorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// describes what to do with stencil after rendering
	swapChainColorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// image data layout before render pass starts
	swapChainColorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// image data layout after render pass

	// Swapchain Color attachment Reference
	VkAttachmentReference swapChainColorAttachmentRef = {};
	swapChainColorAttachmentRef.attachment = 0;
	swapChainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// References to attachments that subpass will take input from
	std::array<VkAttachmentReference, 2> inputReferences;
	inputReferences[0].attachment = 1;
	inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[1].attachment = 2;
	inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Set up subpass 2
	subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[1].colorAttachmentCount = 1;
	subpasses[1].pColorAttachments = &swapChainColorAttachmentRef;
	subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
	subpasses[1].pInputAttachments = inputReferences.data();

	// SUBPASS DEPENDENCIES
	// Need to determine when layout transition occurs using subpass dependencies
	std::array<VkSubpassDependency, 3> subpassDependencies;

	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL & VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	// Transition must happen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// stage access mask (memory access)
	// But must happen before...
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// Subpass 1 layout (color+depth) to subpass 2 layout (shader read) i.e.
	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL & VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL to 
	// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstSubpass = 1;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen after...
	subpassDependencies[2].srcSubpass = 1;
	subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
	// But must happen before...
	subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[2].dependencyFlags = 0;

	// Render pass!
	std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapChainColorAttachmentDesc, colorAttachmentDesc, depthAttachmentDesc };

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();
	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassCreateInfo.pSubpasses = subpasses.data();
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	if (vkCreateRenderPass(m_vkDevice, &renderPassCreateInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Render Pass");
	}
	else
		LOG_INFO("Created Render Pass!");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateFramebuffers()
{
	// resize framebuffer count to equal swap chain image views count
	m_vecFramebuffers.resize(m_vecSwapchainImageViews.size());

	// create framebuffer for each swap chain image view
	for (uint32_t i = 0; i < m_vecSwapchainImageViews.size(); ++i)
	{
		std::array<VkImageView, 3> attachments = { m_vecSwapchainImageViews[i], m_vecColorBufferImageView[i], m_vecDepthBufferImageView[i] };

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_vkRenderPass;									// Render pass layout the framebuffer will be used with					 
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();							// List of attachments
		framebufferCreateInfo.width = m_vkSwapchainExtent.width;							// frambuffer width
		framebufferCreateInfo.height = m_vkSwapchainExtent.height;							// framebuffer height
		framebufferCreateInfo.layers = 1;													// framebuffer layers
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.pNext = nullptr;

		if (vkCreateFramebuffer(m_vkDevice, &framebufferCreateInfo, nullptr, &m_vecFramebuffers[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create Framebuffer");
		}
		else
			LOG_INFO("Framebuffer created!");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::RecordCommands(uint32_t currentImage)
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bufferBeginInfo.flags = 0;												// buffer cab be re-submitted when it has already been submitted & is awaiting execution!

	// Information about how to begin a render pass (only needed for graphical applications) 
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_vkRenderPass;						// Render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };						// start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = m_vkSwapchainExtent;			// size of region to run render pass on (starting at offset) 

	std::array<VkClearValue, 3> clearValues = {};

	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].color = { 0.6f, 0.65f, 0.4f, 1.0f };
	clearValues[2].depthStencil.depth = 1.0f;

	renderPassBeginInfo.pClearValues = clearValues.data();								// list of clear values
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

	renderPassBeginInfo.framebuffer = m_vecFramebuffers[currentImage];

	// start recording commands to command buffer
	if (vkBeginCommandBuffer(m_vecCommandBuffer[currentImage], &bufferBeginInfo) != VK_SUCCESS)
		LOG_ERROR("Failed to begin recording command buffer...");

	// Begin Render Pass
	vkCmdBeginRenderPass(m_vecCommandBuffer[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind Pipeline to be used in render pass
	vkCmdBindPipeline(m_vecCommandBuffer[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);
	
	// Draw Cubes


	// Draw 3D Meshes! 
	for (uint16_t j = 0; j < m_vecModels.size(); j++)
	{
		Model thisModel = m_vecModels[j];

		// Push constants to given shader stage directly
		vkCmdPushConstants(m_vecCommandBuffer[currentImage], m_vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), glm::value_ptr(thisModel.GetModelMatrix()));

		for (uint32_t k = 0; k < thisModel.GetMeshCount(); k++)
		{
			VkBuffer vertexBuffers[] = { thisModel.GetMesh(k)->getVertexBuffer() };						// Buffers to bind
			VkDeviceSize offsets[] = { 0 };																// offsets into buffers being bound
			vkCmdBindVertexBuffers(m_vecCommandBuffer[currentImage], 0, 1, vertexBuffers, offsets);		// Command to bind vertex buffer before drawing with them

			// bind mesh index buffer, with zero offset & using uint32_t type
			vkCmdBindIndexBuffer(m_vecCommandBuffer[currentImage], thisModel.GetMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Dynamic offset amount
			// uint32_t dynamicOffset = m_uiModelUniformAlignment * j;

			std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_vecDescriptorSets[currentImage], m_vecSamplerDescriptorSets[thisModel.GetMesh(k)->getTexID()] };

			// bind descriptor sets
			vkCmdBindDescriptorSets(m_vecCommandBuffer[currentImage],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_vkPipelineLayout,
				0,
				static_cast<uint32_t>(descriptorSetGroup.size()),
				descriptorSetGroup.data(),
				0,
				nullptr);

			// Execute pipeline
			//vkCmdDraw(m_vecCommandBuffer[i], static_cast<uint32_t>(m_pMesh->getVertexCount()), 1, 0, 0);
			vkCmdDrawIndexed(m_vecCommandBuffer[currentImage], thisModel.GetMesh(k)->getIndexCount(), 1, 0, 0, 0);
		}
	}

	// Start second subpass
	vkCmdNextSubpass(m_vecCommandBuffer[currentImage], VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_vecCommandBuffer[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline2);
	vkCmdBindDescriptorSets(m_vecCommandBuffer[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout2, 0, 1, &m_vecInputDescriptorSets[currentImage], 0, nullptr);

	// Draw full screen triangle
	vkCmdDraw(m_vecCommandBuffer[currentImage], 3, 1, 0, 0);

	// End Render Pass
	vkCmdEndRenderPass(m_vecCommandBuffer[currentImage]);

	// finish recording to command buffer
	if (vkEndCommandBuffer(m_vecCommandBuffer[currentImage]) != VK_SUCCESS)
		LOG_ERROR("Failed to record command buffer!");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
		if (vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &m_vecSemaphoreImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &m_vecSemaphoreRenderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(m_vkDevice, &fenceCreateInfo, nullptr, &m_vecFencesRender[i]) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create Semaphores & fences!");
		}
		else
			LOG_INFO("Created Semaphores & fences!");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateUniformBuffers()
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(m_uboViewProjection);

	// Model buffer size
	//VkDeviceSize modelBufferSize = m_uiModelUniformAlignment * Helper::App::MAX_OBJECTS;

	// one uniform buffer for each image (and by extension command buffer)
	m_vecVPUniformBuffer.resize(m_vecSwapchainImages.size());
	m_vecVPUniformBufferMemory.resize(m_vecSwapchainImages.size());

	//m_vecModelDynamicUniformBuffer.resize(m_vecSwapchainImages.size());
	//m_vecModelDynamicUniformBufferMemory.resize(m_vecSwapchainImages.size());

	// create uniform buffers
	for (uint16_t i = 0; i < m_vecSwapchainImages.size(); i++)
	{
		Helper::Vulkan::CreateBuffer(
			m_pDevice->m_vkPhysicalDevice,
			m_vkDevice,
			vpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_vecVPUniformBuffer[i],
			&m_vecVPUniformBufferMemory[i]);

		//Helper::Vulkan::CreateBuffer(
		//							m_vkPhysicalDevice,
		//							m_vkDevice,
		//							modelBufferSize,
		//							VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//							&m_vecModelDynamicUniformBuffer[i],
		//							&m_vecModelDynamicUniformBufferMemory[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateDescriptorPool()
{
	//--- UNIFORM DESCRIPTOR POOL
	// type of descriptors & number of descriptors
	// ViewProjection Pool
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolSize.descriptorCount = static_cast<uint32_t>(m_vecVPUniformBuffer.size());

	// Model Pool
	// VkDescriptorPoolSize modelPoolSize = {};
	// modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// modelPoolSize.descriptorCount = static_cast<uint32_t>(m_vecModelDynamicUniformBuffer.size());

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

	// data to create descriptor pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_vecSwapchainImages.size());		// maximum number of descriptor sets that can be created from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());   // amount of pool sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();								// Pool sizes to create pool with

	if (vkCreateDescriptorPool(m_vkDevice, &poolCreateInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Descriptor Pool");

	//--- SAMPLER DESCRIPTOR POOL
	// Texture sampler pool
	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = Helper::App::MAX_OBJECTS;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = Helper::App::MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	if (vkCreateDescriptorPool(m_vkDevice, &samplerPoolCreateInfo, nullptr, &m_vkSamplerDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Sampler Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Sampler Descriptor Pool");

	// INPUT ATTACHMENT DESCRIPTOR POOL
	// Color attachment 
	VkDescriptorPoolSize colorInputPoolSize = {};
	colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	colorInputPoolSize.descriptorCount = static_cast<uint32_t>(m_vecColorBufferImageView.size());

	// Depth attachment 
	VkDescriptorPoolSize depthInputPoolSize = {};
	depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	depthInputPoolSize.descriptorCount = static_cast<uint32_t>(m_vecDepthBufferImageView.size());

	std::vector<VkDescriptorPoolSize> inputPoolSizes = { colorInputPoolSize, depthInputPoolSize };

	// Create input attachment pool
	VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
	inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	inputPoolCreateInfo.maxSets = m_vecSwapchainImages.size();
	inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
	inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

	if (vkCreateDescriptorPool(m_vkDevice, &inputPoolCreateInfo, nullptr, &m_vkInputDescriptorPool) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Input Descriptor Pool");
	}
	else
		LOG_DEBUG("Successfully created Input Descriptor Pool");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateDescriptorSets()
{
	// Resize descriptor sets so one for every buffer
	m_vecDescriptorSets.resize(m_vecSwapchainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_vecSwapchainImages.size(), m_vkDescriptorSetLayout);

	// Descriptor set allocation info 
	VkDescriptorSetAllocateInfo	setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkDescriptorPool;										// pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_vecSwapchainImages.size());	// number of sets to allocate
	setAllocInfo.pSetLayouts = setLayouts.data();											// layouts to use to allocate sets

	if (vkAllocateDescriptorSets(m_vkDevice, &setAllocInfo, m_vecDescriptorSets.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocated Descriptor sets");
	}
	else
		LOG_DEBUG("Successfully created Descriptor sets");

	// Update all the descriptor set bindings
	for (uint16_t i = 0; i < m_vecSwapchainImages.size(); i++)
	{
		// VIEW PROJECTION DESCRIPTOR
		// buffer info & data offset info
		VkDescriptorBufferInfo vpbufferInfo = {};
		vpbufferInfo.buffer = m_vecVPUniformBuffer[i];								// buffer to get data from
		vpbufferInfo.offset = 0;													// position of start of data
		vpbufferInfo.range = sizeof(UBOViewProjection);								// size of data

		// Data about connection between binding & buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = m_vecDescriptorSets[i];									// Decriptor set to update
		vpSetWrite.dstBinding = 0;													// binding to update
		vpSetWrite.dstArrayElement = 0;												// index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;				// type of descriptor
		vpSetWrite.descriptorCount = 1;												// amount to update		
		vpSetWrite.pBufferInfo = &vpbufferInfo;

		// MODEL DESCRIPTOR
		// VkDescriptorBufferInfo modelBufferInfo = {};
		// modelBufferInfo.buffer = m_vecModelDynamicUniformBuffer[i];
		// modelBufferInfo.offset = 0;
		// modelBufferInfo.range = m_uiModelUniformAlignment;
		// 
		// VkWriteDescriptorSet modelSetWrite = {};
		// modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// modelSetWrite.dstSet = m_vecDescriptorSets[i];
		// modelSetWrite.dstBinding = 1;
		// modelSetWrite.dstArrayElement = 0;
		// modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		// modelSetWrite.descriptorCount = 1;
		// modelSetWrite.pBufferInfo = &modelBufferInfo;

		// List of Descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		// udpate the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(m_vkDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateInputDescriptorSets()
{
	// Resize array to hold descriptor set for each swap chain image
	m_vecInputDescriptorSets.resize(m_vecSwapchainImages.size());

	// Fill array of layouts ready for set creation
	std::vector<VkDescriptorSetLayout> setLayouts(m_vecSwapchainImages.size(), m_vkInputSeLayout);

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkInputDescriptorPool;
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_vecSwapchainImages.size());
	setAllocInfo.pSetLayouts = setLayouts.data();

	// Allocate Descriptor Sets
	if (vkAllocateDescriptorSets(m_vkDevice, &setAllocInfo, m_vecInputDescriptorSets.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to allocated Input Descriptor sets");
	}
	else
		LOG_DEBUG("Successfully created Input Descriptor sets");

	// Update each descriptor set with input attachment
	for (uint32_t i = 0; i < m_vecSwapchainImages.size(); i++)
	{
		// color attachment descriptor
		VkDescriptorImageInfo colorAttachmentDescriptor = {};
		colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorAttachmentDescriptor.imageView = m_vecColorBufferImageView[i];
		colorAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// Color attachment descriptor write
		VkWriteDescriptorSet colorWrite = {};
		colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		colorWrite.dstSet = m_vecInputDescriptorSets[i];
		colorWrite.dstBinding = 0;
		colorWrite.dstArrayElement = 0;
		colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colorWrite.descriptorCount = 1;
		colorWrite.pImageInfo = &colorAttachmentDescriptor;

		// depth attachment descriptor
		VkDescriptorImageInfo depthAttachmentDescriptor = {};
		depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentDescriptor.imageView = m_vecDepthBufferImageView[i];
		depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

		// depth attachment descriptor write
		VkWriteDescriptorSet depthWrite = {};
		depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		depthWrite.dstSet = m_vecInputDescriptorSets[i];
		depthWrite.dstBinding = 1;
		depthWrite.dstArrayElement = 0;
		depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthWrite.descriptorCount = 1;
		depthWrite.pImageInfo = &depthAttachmentDescriptor;

		// List of input descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { colorWrite, depthWrite };

		// Update descriptor sets
		vkUpdateDescriptorSets(m_vkDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char* VulkanRenderer::LoadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
{
	// Number of channels in image
	int channels;

	std::string fileLoc = "Textures/" + fileName;

	// Load pixel data for an image
	unsigned char* imageData = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);
	if (!imageData)
	{
		LOG_ERROR(("Failed to load a Texture file! (" + fileName + ")").c_str());
	}

	// Calculate image size using given data
	*imageSize = *width * *height * 4;

	return imageData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VulkanRenderer::CreateTextureImage(std::string fileName)
{
	int width, height;
	VkDeviceSize imageSize;

	stbi_uc* imageData = LoadTextureFile(fileName, &width, &height, &imageSize);

	// Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;

	// create stafging buffer to hold the loaded data, ready to copy to device!
	Helper::Vulkan::CreateBuffer(m_pDevice->m_vkPhysicalDevice,
		m_vkDevice,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer,
		&imageStagingBufferMemory);

	// copy image data to staging buffer
	void* data;
	vkMapMemory(m_vkDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imageData, static_cast<uint32_t>(imageSize));
	vkUnmapMemory(m_vkDevice, imageStagingBufferMemory);

	// Free original image data
	stbi_image_free(imageData);

	// Create image to hold final texture
	VkImage texImage;
	VkDeviceMemory texImageMemory;

	texImage = Helper::Vulkan::CreateImage(m_pDevice->m_vkPhysicalDevice,
		m_vkDevice,
		width,
		height,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&texImageMemory);

	// Transition image to be DST for copy operation
	Helper::Vulkan::TransitionImageLayout(m_vkDevice,
		m_pDevice->m_vkQueueGraphics,
		m_pDevice->m_vkCommandPoolGraphics,
		texImage,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// COPY DATA TO IMAGE
	Helper::Vulkan::CopyImageBuffer(m_vkDevice, m_pDevice->m_vkQueueGraphics, m_pDevice->m_vkCommandPoolGraphics, imageStagingBuffer, texImage, width, height);

	// Transition image to be shader readable for shader usage
	Helper::Vulkan::TransitionImageLayout(m_vkDevice,
		m_pDevice->m_vkQueueGraphics,
		m_pDevice->m_vkCommandPoolGraphics,
		texImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Add texture data to vector for reference
	m_vecTextureImages.push_back(texImage);
	m_vecTextureImageMemory.push_back(texImageMemory);

	// Destroy staging buffers
	vkDestroyBuffer(m_vkDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(m_vkDevice, imageStagingBufferMemory, nullptr);

	// return index of new texture image
	return m_vecTextureImages.size() - 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VulkanRenderer::CreateTexture(std::string fileName)
{
	// Create texture image & get it's location in an array
	int textureImageLoc = CreateTextureImage(fileName);

	// Create Image view & add to the list
	VkImageView imageView = Helper::Vulkan::CreateImageView(m_vkDevice,
		m_vecTextureImages[textureImageLoc],
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT);

	m_vecTextureImageViews.push_back(imageView);

	// Create Texture Descriptor
	int descriptorLoc = CreateTextureDescriptor(imageView);

	// Return location of set with texture
	return descriptorLoc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateTextureSampler()
{
	// Sampler creation Info
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;								// how to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;								// how to render when image is minified on screen			
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;			// how to handle texture wrap in U (x) direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;			// how to handle texture wrap in V (y) direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;			// how to handle texture wrap in W (z) direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;			// border beyond texture (only works for border clamp)
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;						// whether values of texture coords between [0,1] i.e. normalized
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;				// Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;										// Level of detail bias for mip level
	samplerCreateInfo.minLod = 0.0f;											// minimum level of detail to pick mip level
	samplerCreateInfo.maxLod = 0.0f;											// maximum level of detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_FALSE;								// Enable Anisotropy or not? Check physical device features to see if anisotropy is supported or not!
	samplerCreateInfo.maxAnisotropy = 16;										// Anisotropy sample level

	if (vkCreateSampler(m_vkDevice, &samplerCreateInfo, nullptr, &m_vkTextureSampler) != VK_SUCCESS)
	{ 
		LOG_ERROR("Failed to create Texture sampler!");
	}
	else
		LOG_DEBUG("Successfully created Texture sampler");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VulkanRenderer::CreateTextureDescriptor(VkImageView textureImage)
{
	VkDescriptorSet descriptorSet;

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_vkSamplerDescriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &m_vkSamplerDescriptorSetLayout;

	// allocate descriptor sets
	if (vkAllocateDescriptorSets(m_vkDevice, &setAllocInfo, &descriptorSet) != VK_SUCCESS)
		LOG_ERROR("Failed to allocate Texture Descriptor Sets!");

	// Texture image info
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;				// Image layout when in use
	imageInfo.imageView = textureImage;												// image to bind to set
	imageInfo.sampler = m_vkTextureSampler;											// sampler to use for the set

	// Descriptor write info
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	// Update new descriptor set
	vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorWrite, 0, nullptr);

	// Add descriptor set to list
	m_vecSamplerDescriptorSets.push_back(descriptorSet);

	// Return descriptor set location
	return m_vecSamplerDescriptorSets.size() - 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CreateModel(const std::string& fileName)
{
	// Import Model scene
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
	if (!scene)
		LOG_ERROR("Failed to load model!");

	// Get vector of all materials with 1:1 ID placement
	std::vector<std::string> textureNames = Model::LoadMaterials(scene);

	// conversion from the materials list IDs to our descriptor array IDs
	std::vector<int> listMatToTexture(textureNames.size());

	// Loop over texture names & create textures for them!
	for (uint32_t i = 0; i < listMatToTexture.size(); i++)
	{
		// if material had not texture, set '0' to indicate no texture, texture 0 will be reserved for a default texture
		if (textureNames[i].empty())
		{
			listMatToTexture[i] = 0;
		}
		else
		{
			// Otherwise, create texture & set value to index of new texture
			listMatToTexture[i] = CreateTexture(textureNames[i]);
		}
	}

	// Load all our meshes!
	std::vector<Mesh> vecMeshes = Model::LoadNode(
		m_pDevice,
		m_vkDevice,
		scene->mRootNode,
		scene,
		listMatToTexture);

	Model modelObject = Model(vecMeshes);
	m_vecModels.push_back(modelObject);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{
	// Copy View Projection data
	void* data;
	vkMapMemory(m_vkDevice, m_vecVPUniformBufferMemory[imageIndex], 0, sizeof(UBOViewProjection), 0, &data);
	memcpy(data, &m_uboViewProjection, sizeof(UBOViewProjection));
	vkUnmapMemory(m_vkDevice, m_vecVPUniformBufferMemory[imageIndex]);

	// Copy Model data
	// for (uint32_t i = 0; i < m_vecMeshes.size(); i++)
	// {
	// 	UBOModel* thisModel = (UBOModel*)((uint64_t)m_pModelTransferSpace + (i * m_uiModelUniformAlignment));
	// 	*thisModel = m_vecMeshes[i].GetModel();
	// }
	// 
	// // map the list of model data!
	// vkMapMemory(m_vkDevice, m_vecModelDynamicUniformBufferMemory[imageIndex], 0, m_uiModelUniformAlignment * m_vecMeshes.size(), 0, &data);
	// memcpy(data, m_pModelTransferSpace, m_uiModelUniformAlignment * m_vecMeshes.size());
	// vkUnmapMemory(m_vkDevice, m_vecModelDynamicUniformBufferMemory[imageIndex]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1. Acquire the next available image from the swap chain to draw. Set something to signal when we're finished with the image (semaphore)
// 2. Submit the command buffer to queue for execution, making sure it waits for the image to be signalled as available before drawing & signals when it has finished rendering!
// 3. Present image to the screen when it has signalled finished rendering!
void VulkanRenderer::Render()
{
	// 1. Acquire next image from the swap chain!
	// Wait for given fence to signal (open) from last draw call before continuing...
	vkWaitForFences(m_vkDevice, 1, &m_vecFencesRender[m_uiCurrentFrame], VK_TRUE, UINT64_MAX);

	// Manually reset (close) fence!
	vkResetFences(m_vkDevice, 1, &m_vecFencesRender[m_uiCurrentFrame]);

	// Get index of next image to be drawn to & signal semaphore when ready to be drawn to
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, UINT64_MAX, m_vecSemaphoreImageAvailable[m_uiCurrentFrame], VK_NULL_HANDLE, &imageIndex);

	RecordCommands(imageIndex);
	UpdateUniformBuffers(imageIndex);

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

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;														// Number of semaphores to wait on
	submitInfo.pWaitSemaphores = waitSemaphores;											// List of semaphores to wait on

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;												// stages to check sepmaphores at

	submitInfo.commandBufferCount = 1;														// number of command buffers to submit
	submitInfo.pCommandBuffers = &m_vecCommandBuffer[imageIndex];							// command buffers to submit
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
	presentInfo.swapchainCount = 1;															// number of swapchains to present to
	presentInfo.pSwapchains = &m_vkSwapchain;												// swapchain to present image to
	presentInfo.pImageIndices = &imageIndex;												// index of images in swapchains to present
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::AllocateDynamicBufferTransferSpace()
{
	// calcualte alignment for model data!
	//m_uiModelUniformAlignment = (sizeof(UBOModel) + m_uiMinUniformBufferOffset - 1) & ~(m_uiMinUniformBufferOffset - 1);
	//
	//// Create space in memory to hold dynamic buffer that is aligned to our required alignment & holds MAX_OBJECTS!
	//m_pModelTransferSpace = (UBOModel*)_aligned_malloc(m_uiModelUniformAlignment * Helper::App::MAX_OBJECTS, m_uiModelUniformAlignment);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::CleanupOnWindowResize()
{
	// Cleanup depth related buffers, textures, memory etc.
	for (uint16_t i = 0; i < m_vecDepthBufferImage.size(); i++)
	{
		vkDestroyImageView(m_vkDevice, m_vecDepthBufferImageView[i], nullptr);
		vkDestroyImage(m_vkDevice, m_vecDepthBufferImage[i], nullptr);
		vkFreeMemory(m_vkDevice, m_vecDepthBufferImageMemory[i], nullptr);
	}

	// Cleanup color related buffers, textures, memory etc.
	for (uint16_t i = 0; i < m_vecColorBufferImage.size(); i++)
	{
		vkDestroyImageView(m_vkDevice, m_vecColorBufferImageView[i], nullptr);
		vkDestroyImage(m_vkDevice, m_vecColorBufferImage[i], nullptr);
		vkFreeMemory(m_vkDevice, m_vecColorBufferImageMemory[i], nullptr);
	}

	// Destroy framebuffers!
	for (uint32_t i = 0; i < m_vecFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(m_vkDevice, m_vecFramebuffers[i], nullptr);
	}

	// clean-up existing command buffer & reuse existing pool to allocate new command buffers instead of recreating it!
	vkFreeCommandBuffers(m_vkDevice, m_pDevice->m_vkCommandPoolGraphics, static_cast<uint32_t>(m_vecCommandBuffer.size()), m_vecCommandBuffer.data());

	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline2, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout2, nullptr);

	vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
	vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);

	for (uint32_t i = 0; i < m_vecSwapchainImageViews.size(); ++i)
	{
		vkDestroyImageView(m_vkDevice, m_vecSwapchainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);

	for (uint16_t i = 0; i < m_vecSwapchainImages.size(); i++)
	{
		vkDestroyBuffer(m_vkDevice, m_vecVPUniformBuffer[i], nullptr);
		vkFreeMemory(m_vkDevice, m_vecVPUniformBufferMemory[i], nullptr);

		//vkDestroyBuffer(m_vkDevice, m_vecModelDynamicUniformBuffer[i], nullptr);
		//vkFreeMemory(m_vkDevice, m_vecModelDynamicUniformBufferMemory[i], nullptr);
	}

	//vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
	//vkDestroyDescriptorPool(m_vkDevice, m_vkSamplerDescriptorPool, nullptr);
	//vkDestroyDescriptorPool(m_vkDevice, m_vkInputDescriptorPool, nullptr);

	LOG_DEBUG("Old SwapChain Cleanup");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanRenderer::Cleanup()
{
	// Wait until no action being run on device before destroying! 
	vkDeviceWaitIdle(m_vkDevice);

	CleanupOnWindowResize();

	//_aligned_free(m_pModelTransferSpace);

	//vkDestroyDescriptorPool(m_vkDevice, m_vkInputDescriptorPool, nullptr);
	//vkDestroyDescriptorSetLayout(m_vkDevice, m_vkInputSeLayout, nullptr);

	vkDestroySampler(m_vkDevice, m_vkTextureSampler, nullptr);

	for (uint16_t i = 0; i < m_vecTextureImages.size(); i++)
	{
		vkDestroyImageView(m_vkDevice, m_vecTextureImageViews[i], nullptr);
		vkDestroyImage(m_vkDevice, m_vecTextureImages[i], nullptr);
		vkFreeMemory(m_vkDevice, m_vecTextureImageMemory[i], nullptr);
	}


	vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
	vkDestroyDescriptorPool(m_vkDevice, m_vkSamplerDescriptorPool, nullptr);
	vkDestroyDescriptorPool(m_vkDevice, m_vkInputDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkSamplerDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_vkInputSeLayout, nullptr);


	for (uint32_t i = 0; i < m_vecModels.size(); i++)
	{
		m_vecModels[i].DestroyModel();
	}

	for (uint16_t i = 0; i < m_vecMeshes.size(); i++)
	{
		m_vecMeshes[i].Cleanup();
	}

	// Destroy semaphores
	for (uint32_t i = 0; i < Helper::App::MAX_FRAME_DRAWS; ++i)
	{
		vkDestroySemaphore(m_vkDevice, m_vecSemaphoreImageAvailable[i], nullptr);
		vkDestroySemaphore(m_vkDevice, m_vecSemaphoreRenderFinished[i], nullptr);
		vkDestroyFence(m_vkDevice, m_vecFencesRender[i], nullptr);
	}

	// Destroy command pool
	vkDestroyCommandPool(m_vkDevice, m_pDevice->m_vkCommandPoolGraphics, nullptr);

	vkDestroyDevice(m_vkDevice, nullptr);

	if (Helper::Vulkan::g_bEnableValidationLayer)
	{
		DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}

