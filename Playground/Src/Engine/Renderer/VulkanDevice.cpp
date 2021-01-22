#include "PlaygroundPCH.h"
#include "VulkanDevice.h"

#include "PlaygroundHeaders.h"
#include "Engine/Helpers/Utility.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
{
	// Application Instance
	m_vkInstance = instance;

	// Window Surface
	m_vkSurface = surface;

	// Physical Device
	m_vkPhysicalDevice = nullptr;
	m_vkDeviceProperties = {};
	m_vkDeviceFeaturesAvailable = {};
	m_vkDeviceFeaturesEnabled = {};
	m_vkDeviceMemoryProps = {};
	m_vkQueueGraphics = nullptr;
	m_vkQueuePresent = nullptr;

	m_vkLogicalDevice = nullptr;
	m_vkCommandPoolGraphics = nullptr;
	m_pQueueFamilyIndices = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VulkanDevice::~VulkanDevice()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanDevice::PickPhysicalDevice()
{
	// List out all the physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

	// if there are NO devices with Vulkan support, no point going forward!
	if (deviceCount == 0)
	{
		LOG_CRITICAL("Failed to find GPU with Vulkan support!");
		return;
	}

	// allocate an array to hold all physical devices handles...
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDevices.data());

	// check if physical device has the Queue family required for needed operations
	for (int i = 0; i < deviceCount; ++i)
	{
		// Find Queue families, look for needed queue families...
		FindQueueFamilies(physicalDevices[i]);
		bool bExtensionsSupported = CheckDeviceExtensionSupport(physicalDevices[i]);

		if (m_pQueueFamilyIndices->isComplete() && bExtensionsSupported)
		{
			m_vkPhysicalDevice = physicalDevices[i];

			vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_vkDeviceProperties);

			LOG_DEBUG("Vendor ID:		{0}", m_vkDeviceProperties.vendorID);
			LOG_DEBUG("Device Name:		{0}", m_vkDeviceProperties.deviceName);
			LOG_DEBUG("Driver Version:	{0}", m_vkDeviceProperties.driverVersion);
			LOG_DEBUG("API Version:		{0}", m_vkDeviceProperties.apiVersion);

			break;
		}
	}

	// If we don't find suitable device, return!
	if (m_vkPhysicalDevice == VK_NULL_HANDLE)
	{
		LOG_ERROR("Failed to find suitable GPU!");
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get count of total number of extensions
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// gather their information
	m_vecSupportedExtensions.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, m_vecSupportedExtensions.data());

	// Compare Required extensions with supported extensions...
	bool bExtensionFound = false;
	for (int i = 0; i < Helper::Vulkan::g_strDeviceExtensions.size(); ++i)
	{
		for (int j = 0; j < extensionCount; ++j)
		{
			// If device supported extensions matches the one we want, good news ... Enumarate them!
			if (strcmp(Helper::Vulkan::g_strDeviceExtensions[i], m_vecSupportedExtensions[j].extensionName) == 0)
			{
				bExtensionFound = true;

				std::string msg = std::string(Helper::Vulkan::g_strDeviceExtensions[i]) + " device extension found!";
				LOG_DEBUG(msg.c_str());

				break;
			}
		}

		// No matching extension found ... bail out!
		if (!bExtensionFound)
		{
			return false;
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanDevice::FindQueueFamilies(VkPhysicalDevice device)
{
	// retrieve list of queue families 
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	m_pQueueFamilyIndices = new QueueFamilyIndices();

	// VkQueueFamilyProperties contains details about the queue family. We need to find at least one
	// queue family that supports VK_QUEUE_GRAPHICS_BIT
	for (int i = 0; i < queueFamilyCount; ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_pQueueFamilyIndices->m_uiGraphicsFamily = i;
		}

		// check if this queue family has capability of presenting to our window surface!
		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &bPresentSupport);

		// if yes, store presentation family queue index!
		if (bPresentSupport)
		{
			m_pQueueFamilyIndices->m_uiPresentFamily = i;
		}

		if (m_pQueueFamilyIndices->isComplete())
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanDevice::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// std::set allows only One unique value for input values, no duplicate is allowed, so if both Graphics Queue family
	// and Presentation Queue family index is same then it will avoid the duplicates and assign only one queue index!
	std::set<uint32_t> uniqueQueueFamilies =
	{
		m_pQueueFamilyIndices->m_uiGraphicsFamily.value(),
		m_pQueueFamilyIndices->m_uiPresentFamily.value()
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

	if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkLogicalDevice) != VK_SUCCESS)
	{
		LOG_CRITICAL("Failed to create logical vulkan device!");
		return;
	}

	// The queues are automatically created along with the logical device, but we don't 
	// have a handle to interface with them yet. Since we are only creating a single queue
	// from this family, we will use index 0
	vkGetDeviceQueue(m_vkLogicalDevice, m_pQueueFamilyIndices->m_uiGraphicsFamily.value(), 0, &m_vkQueueGraphics);
	vkGetDeviceQueue(m_vkLogicalDevice, m_pQueueFamilyIndices->m_uiPresentFamily.value(), 0, &m_vkQueuePresent);

	LOG_INFO("Logical Device Created!");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanDevice::CreateGraphicsCommandPool(VkDevice logicalDevice)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_pQueueFamilyIndices->m_uiGraphicsFamily.value();
	commandPoolCreateInfo.pNext = nullptr;

	// Create a Graphics queue family Command Pool
	if (vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &m_vkCommandPoolGraphics) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Command Pool!");
	}
	else
		LOG_DEBUG("Created Command Pool!");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanDevice::CreateGraphicsCommandBuffers(uint32_t size, VkDevice logicalDevice)
{
	m_vecCommandBuffer.resize(size);

	VkCommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.commandPool = m_vkCommandPoolGraphics;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;						// Buffer you submit directly to the queue. can't be called by other buffers!
																						// BUFFER_LEVEL_SECONDARY can't be called directly but can be called from other buffers via "vkCmdExecuteCommands"
	commandBufferAllocInfo.commandBufferCount = (uint32_t)m_vecCommandBuffer.size();
	commandBufferAllocInfo.pNext = nullptr;

	// Allocate command buffers & place handles in array of buffers!
	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocInfo, m_vecCommandBuffer.data()) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Command buffer!");
	}
	else
		LOG_INFO("Created Command buffers!");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//--- Find suitable memory type based on allowed type & property flags
uint32_t VulkanDevice::FindMemoryTypeIndex(uint32_t allowedTypeIndex, VkMemoryPropertyFlags props)
{
	// Get properties of physical device memory
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkDeviceMemoryProps);

	for (uint32_t i = 0; i < m_vkDeviceMemoryProps.memoryTypeCount; i++)
	{
		if ((allowedTypeIndex & (1 << i))												// Index of memory type must match corresponding bit in allowed types!
			&& (m_vkDeviceMemoryProps.memoryTypes[i].propertyFlags & props) == props)	// Desired property bit flags are part of the memory type's property flags!
		{
			// This memory type is valid, so return index!
			return i;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VulkanDevice::Cleanup()
{
	
}
