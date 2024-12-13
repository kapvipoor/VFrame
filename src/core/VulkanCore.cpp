#include <assert.h>
#include <fstream>
#include <sstream>

#include "VulkanCore.h"

#if	VULKAN_DEBUG == 1
VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data)
{
	if (!callback_data || !callback_data->pMessageIdName || !callback_data->pMessage)
		return VK_FALSE;

	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		std::cout << "Vulkan Info: "
			<< "{" << callback_data->messageIdNumber << "} - "
			<< "{" << callback_data->pMessageIdName << "}: "
			<< "{" << callback_data->pMessage << std::endl;
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cout << "Vulkan Warning: "
			<< "{" << callback_data->messageIdNumber << "} - "
			<< "{" << callback_data->pMessageIdName << "}: "
			<< "{" << callback_data->pMessage << std::endl;
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		std::cout << "Vulkan Error: "
			<< "{" << callback_data->messageIdNumber << "} - "
			<< "{" << callback_data->pMessageIdName << "}: "
			<< "{" << callback_data->pMessage << std::endl;
	}
	return VK_FALSE;
}

VKAPI_ATTR VkBool32	VKAPI_CALL
debugReportCallback(
	VkDebugReportFlagsEXT		p_msgFlags,
	VkDebugReportObjectTypeEXT	p_ObjType,
	uint64_t					p_SrcObj,
	size_t						p_location,
	int32_t						p_MesgCode,
	const char* p_layerPrefix,
	const char* p_Message,
	void* pUserData)
{
	std::ostringstream stream;
	stream << "[@ " << p_layerPrefix << "] " << p_Message;

#if VULKAN_SHOW_MESSAGES == 1
	if (p_msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		std::cerr << "Vulkan Error " << stream.str() << std::endl;

#elif VULKAN_SHOW_MESSAGES == 2
	if (p_msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		std::cerr << "Vulkan Error " << stream.str() << std::endl;

	else if (p_msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::cerr << "Vulkan Warning: " << stream.str() << std::endl;

	else if (p_msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		std::cerr << "Vulkan Performance Warning: " << stream.str() << std::endl;

#elif VULKAN_SHOW_MESSAGES == 3
	if (p_msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		std::cerr << "Vulkan Information: " << stream.str() << std::endl;

	else if (p_msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::cerr << "Vulkan Warning: " << stream.str() << std::endl;

	else if (p_msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		std::cerr << "Vulkan Performance Warning: " << stream.str() << std::endl;

	else if (p_msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		std::cerr << "Vulkan Error: " << stream.str() << std::endl;

	else if (p_msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		std::cerr << "Vulkan Debug Report: " << stream.str() << std::endl;

	else
		std::cerr << "Vulkan Info " << stream.str() << std::endl;
#endif

	return false;
}

#endif

CVulkanCore::CVulkanCore(const char* p_applicaitonName, int p_renderWidth, int p_renderHeight)
		: m_applicationName(p_applicaitonName)
		, m_renderWidth(p_renderWidth)
		, m_renderHeight(p_renderHeight)
		, m_screenWidth(p_renderWidth)
		, m_screenHeight(p_renderHeight)
		, m_vkInstance(VK_NULL_HANDLE)
		, m_vkDevice(VK_NULL_HANDLE)
		, m_vkPhysicalDevice(VK_NULL_HANDLE)
		, m_QFIndex(0)
		, m_vkSurface(VK_NULL_HANDLE)
{}

CVulkanCore::~CVulkanCore()
{
}

void CVulkanCore::BeginDebugMarker(VkCommandBuffer p_vkCmdBuff, const char* pMsg)
{
#if VULKAN_DEBUG_MARKERS == 1
	if(!m_fpvkCmdBeginDebugUtilsLabel)
		m_fpvkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_vkDevice, "vkCmdBeginDebugUtilsLabelEXT");

	VkDebugUtilsLabelEXT debugLabel{};
	debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	debugLabel.pLabelName = pMsg;
	debugLabel.color[0] = 1.0f;
	debugLabel.color[1] = 0.14f;
	debugLabel.color[2] = 0.14f;
	debugLabel.color[3] = 1.0f;
	m_fpvkCmdBeginDebugUtilsLabel(p_vkCmdBuff, &debugLabel);
#endif
}

void CVulkanCore::EndDebugMarker(VkCommandBuffer p_vkCmdBuff)
{
#if VULKAN_DEBUG_MARKERS == 1
	if(!m_fpvkCmdEndDebugUtilsLabelEXT)
		m_fpvkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_vkDevice, "vkCmdEndDebugUtilsLabelEXT");

	m_fpvkCmdEndDebugUtilsLabelEXT(p_vkCmdBuff);
#endif
}

void CVulkanCore::InsertMarker(VkCommandBuffer p_vkCmdBuff, const char* pMsg)
{
#if VULKAN_DEBUG_MARKERS == 1
	if(!m_fpvkCmdInsertDebugUtilsLabelEXT)
		m_fpvkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_vkDevice, "vkCmdInsertDebugUtilsLabelEXT");

	VkDebugUtilsLabelEXT debugLabel{};
	debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	debugLabel.pLabelName = pMsg;
	debugLabel.color[0] = 1.0f;
	debugLabel.color[1] = 0.14f;
	debugLabel.color[2] = 0.14f;
	debugLabel.color[3] = 1.0f;
	m_fpvkCmdInsertDebugUtilsLabelEXT(p_vkCmdBuff, &debugLabel);
#endif
}

void CVulkanCore::SetDebugName(uint64_t pObject, VkObjectType pObjectType, const char* pName)
{
#if VULKAN_DEBUG_MARKERS == 1
	if(!m_fpVkSetDebugUtilsObjectName)
		m_fpVkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_vkDevice, "vkSetDebugUtilsObjectNameEXT");

	VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	name_info.objectType = pObjectType;
	name_info.objectHandle = pObject;
	name_info.pObjectName = pName;
	m_fpVkSetDebugUtilsObjectName(m_vkDevice, &name_info);
#endif
}

void CVulkanCore::cleanUp()
{
	for (auto& view : m_swapchainImageViewList)
		vkDestroyImageView(m_vkDevice, view, nullptr);

	vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);

#ifdef VULKAN_DEBUG
	if (m_debugUtilsMessenger != VK_NULL_HANDLE)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT pfnvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (!pfnvkDestroyDebugUtilsMessengerEXT)
		{
			std::cerr << "Could not load function pointer for vkCreateDebugReportCallbackEXT." << std::endl;
		}
		pfnvkDestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugUtilsMessenger, nullptr);
	}
	if (m_debugReportCallback != VK_NULL_HANDLE)
	{
		PFN_vkDestroyDebugReportCallbackEXT pfnvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugReportCallbackEXT");
		if (!pfnvkDestroyDebugReportCallbackEXT)
		{
			std::cerr << "Could not load function pointer for vkCreateDebugReportCallbackEXT." << std::endl;
		}
		pfnvkDestroyDebugReportCallbackEXT(m_vkInstance, m_debugReportCallback, nullptr);
	}
#endif
	vkDestroyInstance(m_vkInstance, nullptr);
}

bool CVulkanCore::initialize(const InitData& p_initData)
{
	if (!CreateInstance(m_applicationName))
		return false;

	if (!CreateDevice(p_initData.queueType))
		return false;

	if (!CreateSurface(p_initData.winInstance, p_initData.winHandle))
		return false;

	if (!CreateSwapChain(p_initData.swapchainImageFormat, p_initData.swapChaineImageUsage))
		return false;

	if (!CreateSwapChainImages(p_initData.swapchainImageFormat))
		return false;

	return true;
}

bool CVulkanCore::CreateInstance(const char* p_applicaitonName)
{
	bool hasDebugUtils = true;

	// Brute force setting up instance layers and extensions; instance creation will fail if layers are
	// not supported
	std::vector<const char*> instanceExtensionList;
	instanceExtensionList.push_back("VK_KHR_win32_surface");
	instanceExtensionList.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	instanceExtensionList.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#if VULKAN_DEBUG == 1
		instanceExtensionList.push_back("VK_EXT_debug_utils");
		instanceExtensionList.push_back("VK_EXT_debug_report");
#endif

	ListAvailableInstanceExtensions(instanceExtensionList);

	std::vector<const char*> instanceLayerList;
	//instanceLayerList.push_back("VK_LAYER_RENDERDOC_Capture");
#if VULKAN_DEBUG == 1
	instanceLayerList.push_back("VK_LAYER_LUNARG_monitor");
	instanceLayerList.push_back("VK_LAYER_KHRONOS_validation");
	ListAvailableInstanceLayers(instanceLayerList);
#endif

	 
	VkInstanceCreateInfo instanceCreateInfo{};

	// Set up vulkan debug callback pipeline
#if VULKAN_DEBUG == 1
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo;
	if (hasDebugUtils)
	{
		debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugUtilsCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;

		instanceCreateInfo.pNext = &debugUtilsCreateInfo;
	}
	else
	{
		debugCallbackCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		debugCallbackCreateInfo.pfnCallback = debugReportCallback;
		debugCallbackCreateInfo.flags =
			VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
			VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
			VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_ERROR_BIT_EXT |
			VkDebugReportFlagBitsEXT::VK_DEBUG_REPORT_DEBUG_BIT_EXT;

		instanceCreateInfo.pNext = &debugCallbackCreateInfo;
	}
#endif

	VkApplicationInfo appInfo{};
	appInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = p_applicaitonName;

	instanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = (uint32_t)instanceLayerList.size();
	instanceCreateInfo.ppEnabledLayerNames = instanceLayerList.data();
	instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensionList.size();
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionList.data();

	VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateInstance failed: " << res << std::endl;
		return false;
	}

#if VULKAN_DEBUG == 1
	if (hasDebugUtils)
	{
		PFN_vkCreateDebugUtilsMessengerEXT pfnvkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");
		if(!pfnvkCreateDebugUtilsMessengerEXT)
		{
			std::cerr << "Could not load function pointer for vkCreateDebugUtilsMessengerEXT." << std::endl;
			return false;
		}

		VkResult res = pfnvkCreateDebugUtilsMessengerEXT(m_vkInstance, &debugUtilsCreateInfo, nullptr, &m_debugUtilsMessenger);
		if (res != VK_SUCCESS)
		{
			std::cerr << "Could not create debug utils messenger " << res << std::endl;
			return false;
		}
	}
	else
	{
		PFN_vkCreateDebugReportCallbackEXT pfnvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugReportCallbackEXT");
		if (!pfnvkCreateDebugReportCallbackEXT)
		{
			std::cerr << "Could not load function pointer for vkCreateDebugReportCallbackEXT." << std::endl;
			return false;
		}

		VkResult res = pfnvkCreateDebugReportCallbackEXT(m_vkInstance, &debugCallbackCreateInfo, nullptr, &m_debugReportCallback);
		if (res != VK_SUCCESS)
		{
			std::cerr << "Could not create debug report callback " << res << std::endl;
			return false;
		}
	}
#endif

	return true;
}

bool CVulkanCore::CreateDevice(VkQueueFlagBits p_queueType)
{
	// Get Active physical device - force to select the first available device
	{
		uint32_t deviceCount;
		VkResult res = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
		if (res != VK_SUCCESS)
		{
			std::cerr << "vkEnumeratePhysicalDevices failed: " << res << std::endl;
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDeviceList(deviceCount);
		res = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDeviceList.data());
		if (res != VK_SUCCESS || physicalDeviceList[0] == VK_NULL_HANDLE)
		{
			std::cerr << "vkEnumeratePhysicalDevices failed / physical device not found: " << res << std::endl;
			return false;
		}

		for (const auto& physicalDevice : physicalDeviceList)
		{
			VkPhysicalDeviceProperties deviceProperties{};
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_vkPhysicalDevice = physicalDevice;
			}
		}		
	}

	// Brute-force setting up device extensions; device creation will fail if extensions are
	// not supported 
	std::vector<const char*> deviceExtensionList;
	{
		//deviceExtensionList.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		deviceExtensionList.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
		deviceExtensionList.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		deviceExtensionList.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		deviceExtensionList.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		//deviceExtensionList.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
#if VULKAN_DEBUG_MARKERS == 1
		deviceExtensionList.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
#endif
	}

	// Getting compute queue details only only
	float queuePriority[] = { 1.0f, 1.0f };
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	deviceQueueCreateInfos.resize(1);
	{
		vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkPhysicalDeviceMemProp);

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilyList.data());

		for (uint32_t qfIndex = 0; qfIndex < queueFamilyCount; qfIndex++)
		{
			if (queueFamilyList[qfIndex].queueFlags & p_queueType &&
				queueFamilyList[qfIndex].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				m_QFIndex = qfIndex;
				deviceQueueCreateInfos[0] = VkDeviceQueueCreateInfo{};
				deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				deviceQueueCreateInfos[0].queueCount = 2;
				deviceQueueCreateInfos[0].queueFamilyIndex = m_QFIndex;
				deviceQueueCreateInfos[0].pQueuePriorities = queuePriority;
				break;
			}
		}
	}

	VkPhysicalDeviceFeatures supportedFeatures{};
	vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &supportedFeatures);

	VkPhysicalDeviceFeatures enabledFeatures{};
	enabledFeatures.geometryShader										= VK_TRUE;
	enabledFeatures.fragmentStoresAndAtomics							= VK_TRUE;
	enabledFeatures.shaderStorageImageReadWithoutFormat					= VK_TRUE;
	enabledFeatures.shaderStorageImageWriteWithoutFormat				= VK_TRUE;
	enabledFeatures.vertexPipelineStoresAndAtomics						= VK_TRUE;

	if (enabledFeatures.geometryShader != supportedFeatures.geometryShader &&
		enabledFeatures.fragmentStoresAndAtomics != supportedFeatures.fragmentStoresAndAtomics)
	{
		std::cerr << "CVulkanCore::CreateDevice Error: vkGetPhysicalDeviceFeatures: Requested features not supported by device. " << std::endl;
		return false;
	}

	// Enable support for bindless
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT bindlessDescFeatures{};
	bindlessDescFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	bindlessDescFeatures.shaderSampledImageArrayNonUniformIndexing		= VK_TRUE;
	bindlessDescFeatures.runtimeDescriptorArray							= VK_TRUE;
	bindlessDescFeatures.descriptorBindingVariableDescriptorCount		= VK_TRUE;
	bindlessDescFeatures.descriptorBindingPartiallyBound				= VK_TRUE;
	bindlessDescFeatures.descriptorBindingSampledImageUpdateAfterBind	= VK_TRUE;
	bindlessDescFeatures.descriptorBindingStorageImageUpdateAfterBind	= VK_TRUE;
	bindlessDescFeatures.descriptorBindingUniformBufferUpdateAfterBind	= VK_TRUE;
	bindlessDescFeatures.descriptorBindingStorageBufferUpdateAfterBind  = VK_TRUE;
	bindlessDescFeatures.descriptorBindingUpdateUnusedWhilePending		= VK_TRUE;

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
	physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physicalDeviceFeatures2.features = enabledFeatures;
	physicalDeviceFeatures2.pNext = (void*)&bindlessDescFeatures;
		
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &physicalDeviceFeatures2;
	deviceCreateInfo.pEnabledFeatures = nullptr;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)deviceQueueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensionList.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionList.data();
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;

	VkResult res = vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateDevice failed: " << res << std::endl;
		return false;
	}

	// Primary Render Queue
	{
		vkGetDeviceQueue(m_vkDevice, m_QFIndex, 0, &m_vkQueue[0]);
	}

	// Secondary Queue
	{
		vkGetDeviceQueue(m_vkDevice, m_QFIndex, 1, &m_secondaryQueue);
		if(m_secondaryQueue == VK_NULL_HANDLE)
		{
			std::cerr << "vkGetDeviceQueue failed for secondary queue" << std::endl;
			return false;
		}
	}

	return true;
}

bool CVulkanCore::CreateSurface(HINSTANCE p_hnstns, HWND p_Hwnd)
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hinstance = p_hnstns;
	createInfo.hwnd = p_Hwnd;

	VkResult res = vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkSurface);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateWin32SurfaceKHR failed: " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::CreateSwapChain(VkFormat p_format, VkImageUsageFlags p_imageUsage)
{
	// Brute-forcing swapchain count to 2 without querying for capabilities
	// Forcing presentation mode to immediate without querying for support
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.pNext = nullptr;
	swapChainCreateInfo.surface = m_vkSurface;
	swapChainCreateInfo.minImageCount = FRAME_BUFFER_COUNT;
	swapChainCreateInfo.imageFormat = p_format;
	swapChainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapChainCreateInfo.imageExtent.height = m_renderHeight;
	swapChainCreateInfo.imageExtent.width = m_renderWidth;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = p_imageUsage;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.queueFamilyIndexCount = 0;
	swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	VkResult res = vkCreateSwapchainKHR(m_vkDevice, &swapChainCreateInfo, nullptr, &m_vkSwapchain);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateSwapchainKHR failed: " << res << std::endl;
		return false;
	}

	uint32_t scCount = FRAME_BUFFER_COUNT;
	res = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &scCount, nullptr);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkGetSwapchainImagesKHR failed: " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::CreateSwapChainImages(VkFormat p_format)
{
	uint32_t scCount = FRAME_BUFFER_COUNT;
	VkResult res = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &scCount, m_swapchainImageList);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkGetSwapchainImagesKHR failed: " << res << std::endl;
		return false;
	}

	for (int it = 0; it != FRAME_BUFFER_COUNT; ++it)
	{
		VkImageViewCreateInfo l_vkSwapChainImageViewInfo{};
		l_vkSwapChainImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		l_vkSwapChainImageViewInfo.pNext = nullptr;
		l_vkSwapChainImageViewInfo.image = m_swapchainImageList[it];
		l_vkSwapChainImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		l_vkSwapChainImageViewInfo.format = p_format;		// same as swap chain format
		l_vkSwapChainImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;	//for shader swizzling
		l_vkSwapChainImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;	//for shader swizzling
		l_vkSwapChainImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;	//for shader swizzling
		l_vkSwapChainImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;	//for shader swizzling
		l_vkSwapChainImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		l_vkSwapChainImageViewInfo.subresourceRange.baseMipLevel = 0;
		l_vkSwapChainImageViewInfo.subresourceRange.levelCount = 1;
		l_vkSwapChainImageViewInfo.subresourceRange.baseArrayLayer = 0;
		l_vkSwapChainImageViewInfo.subresourceRange.layerCount = 1;
		res = vkCreateImageView(m_vkDevice, &l_vkSwapChainImageViewInfo, nullptr, &m_swapchainImageViewList[it]);
		if (res != VK_SUCCESS)
		{
			std::cerr << "vkCreateImageView failed: " << res << std::endl;
			return false;
		}
	}

	return true;
}

// for now there is an offline glsl to spirv compiler in place but will later update this with
// a runtime compiler
bool CVulkanCore::LoadShader(const char* p_shaderpath, VkShaderModule& p_shader)
{
	std::size_t found = std::string(p_shaderpath).find_last_of(".");
	if (found == std::string::npos)
	{
		std::cerr << "Invalid shader path - " << p_shaderpath << std::endl;
		return false;
	}

	std::string fileExtension = std::string(p_shaderpath).substr(found + 1);
	if (std::strcmp(fileExtension.c_str(), "spv") != 0)
	{
		std::cerr << "Invalid shader type - " << p_shaderpath << std::endl;
		return false;
	}

	size_t fileSize = 0;
	char* shaderBlob = BinaryLoader(p_shaderpath, fileSize);
	if (shaderBlob == nullptr)
	{
		std::cerr << "Failed to load shader blob - " << p_shaderpath << std::endl;
		return false;
	}

	VkShaderModuleCreateInfo shaderCreateInfo{};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateInfo.codeSize = fileSize;
	shaderCreateInfo.pCode = (uint32_t*)shaderBlob;

	VkResult res = vkCreateShaderModule(m_vkDevice, &shaderCreateInfo, nullptr, &p_shader);
	delete[] shaderBlob;

	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateShaderModule failed: " << res << std::endl;
		return false;
	}

	return true;
}

// as the draw will happen using the compute queue, there is not going to be any renderpass
// when creating the frame buffer
bool CVulkanCore::CreateFramebuffer(VkRenderPass p_renderPass, VkFramebuffer& p_frameBuffer, 
	VkImageView* p_imageViewList, uint32_t p_fbCount, uint32_t p_width, uint32_t p_height)
{
	VkFramebufferCreateInfo framebufferCreateInfo{};
	framebufferCreateInfo.pNext = NULL;
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.attachmentCount = p_fbCount;
	framebufferCreateInfo.pAttachments = p_imageViewList;
	framebufferCreateInfo.renderPass = p_renderPass;
	framebufferCreateInfo.height = p_height;
	framebufferCreateInfo.width = p_width;
	framebufferCreateInfo.layers = 1;

	VkResult res = vkCreateFramebuffer(m_vkDevice, &framebufferCreateInfo, nullptr, &p_frameBuffer);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateFramebuffer failed: " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::DestroyFramebuffer(VkFramebuffer p_framebuffer)
{
	vkDestroyFramebuffer(m_vkDevice, p_framebuffer, nullptr);
}

bool CVulkanCore::AcquireNextSwapChain(VkSemaphore p_semaphore, uint32_t& p_swapChainID)
{
	VkResult res = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, UINT64_MAX, p_semaphore, VK_NULL_HANDLE, &p_swapChainID);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkAcquireNextImageKHR failed " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::CreateCommandPool(uint32_t p_qfIndex, VkCommandPool& p_cmdPool)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo = VkCommandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = p_qfIndex;
	VkResult res = vkCreateCommandPool(m_vkDevice, &commandPoolCreateInfo, nullptr, &p_cmdPool);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateCommandPool failed: " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::ResetCommandPool(VkCommandPool& p_cmdPool)
{
	VkResult res = vkResetCommandPool(m_vkDevice, p_cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkResetCommandPool failed: " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::DestroyCommandPool(VkCommandPool p_cmdPool)
{
	vkDestroyCommandPool(m_vkDevice, p_cmdPool, nullptr);
}

bool CVulkanCore::CreateCommandBuffers(VkCommandPool p_cmdPool, VkCommandBuffer* p_cmdBuffers, uint32_t p_cbCount, std::string* p_debugNames)
{
	VkCommandBufferAllocateInfo cmdBfrAllocInfo{};
	cmdBfrAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBfrAllocInfo.commandPool = p_cmdPool;
	cmdBfrAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBfrAllocInfo.commandBufferCount = p_cbCount;
	VkResult res = vkAllocateCommandBuffers(m_vkDevice, &cmdBfrAllocInfo, p_cmdBuffers);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkAllocateCommandBuffers failed: " << res << std::endl;
		return false;
	}
	
	for (uint32_t i = 0; i < p_cbCount; i++)
	{
		SetDebugName((uint64_t)p_cmdBuffers[i], VK_OBJECT_TYPE_COMMAND_BUFFER, (p_debugNames[i] + " Command Buffer").c_str());
	}	

	return true;
}

bool CVulkanCore::CreateDescriptorPool(VkDescriptorPoolSize* p_dpSizeList, uint32_t p_dpSizeCount, VkDescriptorPool& p_vkdescriptorPool)
{
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	// This is a bindless feature that enables us to write-update the descriptor after binding.
	// The latest write-update is then considered valid.
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	descriptorPoolCreateInfo.maxSets = p_dpSizeCount;
	descriptorPoolCreateInfo.poolSizeCount = p_dpSizeCount;
	descriptorPoolCreateInfo.pPoolSizes = p_dpSizeList;

	VkResult res = vkCreateDescriptorPool(m_vkDevice, &descriptorPoolCreateInfo, nullptr, &p_vkdescriptorPool);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateDescriptorPool failed: " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::DestroyDescriptorPool(VkDescriptorPool p_descPool)
{
	vkDestroyDescriptorPool(m_vkDevice, p_descPool, nullptr);
}

bool CVulkanCore::AllocateDescriptorSets(VkDescriptorPool p_dPool, VkDescriptorSetLayout* p_dslList, uint32_t p_dslCount, VkDescriptorSet* p_vkDescriptorSet, void* p_next)
{
	VkDescriptorSetAllocateInfo dsAllocInfo{};
	dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo.pNext = p_next;
	dsAllocInfo.descriptorPool = p_dPool;
	dsAllocInfo.pSetLayouts = p_dslList;
	dsAllocInfo.descriptorSetCount = p_dslCount;

	VkResult res = vkAllocateDescriptorSets(m_vkDevice, &dsAllocInfo, p_vkDescriptorSet);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkAllocateDescriptorSets failed: " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding* p_dsLayoutList, uint32_t p_dsLayoutCount, 
											VkDescriptorSetLayout& p_vkdsLayout, void* p_next, bool p_isBindless)
{
	VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo{};
	dsLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dsLayoutCreateInfo.pNext = p_next;
	dsLayoutCreateInfo.bindingCount = p_dsLayoutCount;
	dsLayoutCreateInfo.pBindings = p_dsLayoutList;

	if (p_isBindless)
	{
		dsLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	}

	VkResult res = vkCreateDescriptorSetLayout(m_vkDevice, &dsLayoutCreateInfo, nullptr, &p_vkdsLayout);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateDescriptorSetLayout failed: " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::DestroyDescriptorSetLayout(VkDescriptorSetLayout p_descLayput)
{
	vkDestroyDescriptorSetLayout(m_vkDevice, p_descLayput, nullptr);
}

bool CVulkanCore::CreatePipelineLayout(VkPushConstantRange* p_pushConstants, uint32_t p_pcCount,
	VkDescriptorSetLayout* p_descLayouts, uint32_t p_dlCount,
	VkPipelineLayout& p_vkPipelineLayout, std::string p_debugName)
{
	VkPipelineLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pPushConstantRanges = p_pushConstants;
	layoutCreateInfo.pushConstantRangeCount = p_pcCount;
	layoutCreateInfo.pSetLayouts = p_descLayouts;
	layoutCreateInfo.setLayoutCount = p_dlCount;

	VkResult res = vkCreatePipelineLayout(m_vkDevice, &layoutCreateInfo, nullptr, &p_vkPipelineLayout);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreatePipelineLayout failed: " << res << std::endl;
		return false;
	}

	SetDebugName((uint64_t)p_vkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, p_debugName.c_str());

	return true;
}

void CVulkanCore::DestroyPipelineLayout(VkPipelineLayout p_pipeLayout)
{
	vkDestroyPipelineLayout(m_vkDevice, p_pipeLayout, nullptr);
}

bool CVulkanCore::CreateRenderpass(Renderpass& p_rpData)
{
	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.inputAttachmentCount = 0;
	subpassDesc.pInputAttachments = nullptr;
	subpassDesc.colorAttachmentCount = (uint32_t)p_rpData.colorAttachmentRefList.size();
	subpassDesc.pColorAttachments = p_rpData.colorAttachmentRefList.data();
	if(p_rpData.isDepthAttached() == true)
		subpassDesc.pDepthStencilAttachment = &p_rpData.depthAttachemntRef;

	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = 0;
	subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderpassInfo{};
	renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassInfo.attachmentCount = (uint32_t)p_rpData.attachemntDescList.size();
	renderpassInfo.pAttachments = p_rpData.attachemntDescList.data();
	renderpassInfo.subpassCount = 1;
	renderpassInfo.pSubpasses = &subpassDesc;
	renderpassInfo.dependencyCount = 1;
	renderpassInfo.pDependencies = &subpassDependency;

	VkResult res = vkCreateRenderPass(m_vkDevice, &renderpassInfo, nullptr, &p_rpData.renderpass);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateRenderPass failed: " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::SetClearColorValue(Renderpass& p_renderpass, uint32_t p_attachmentIndex, const VkClearColorValue& p_colorClearValue)
{
	p_renderpass.colorClearValue[p_attachmentIndex] = p_colorClearValue;
}

void CVulkanCore::BeginRenderpass(VkFramebuffer p_frameBfr, const Renderpass& p_renderpass, VkCommandBuffer& p_cmdBfr)
{
	std::vector<VkClearValue> clear;
	for (auto colorClear : p_renderpass.colorClearValue)
	{
		VkClearValue cval{};
		cval.color = colorClear;
		clear.push_back(cval);
	}

	if (p_renderpass.depthAttachemntRef.attachment >= 0)
	{
		VkClearValue cval{};
		cval.depthStencil = p_renderpass.depthStencilClearValue;
		clear.push_back(cval);
	}

	VkRenderPassBeginInfo renderpassBegin{};
	renderpassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpassBegin.framebuffer = p_frameBfr;
	renderpassBegin.pClearValues = clear.data();
	renderpassBegin.clearValueCount = (uint32_t)clear.size();
	renderpassBegin.renderArea.extent.width = p_renderpass.framebufferWidth;
	renderpassBegin.renderArea.extent.height = p_renderpass.framebufferHeight;
	renderpassBegin.renderArea.offset = { 0, 0 };
	renderpassBegin.renderPass = p_renderpass.renderpass;
	vkCmdBeginRenderPass(p_cmdBfr, &renderpassBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void CVulkanCore::EndRenderPass(VkCommandBuffer& p_cmdBfr)
{
	vkCmdEndRenderPass(p_cmdBfr);
}

void CVulkanCore::DestroyRenderpass(VkRenderPass p_renderpass)
{
	vkDestroyRenderPass(m_vkDevice, p_renderpass, nullptr);
}

void CVulkanCore::SetViewport(VkCommandBuffer p_cmdbfr, float p_minD, float p_maxD, float p_width, float p_height)
{
	VkViewport view{};
	view.height = p_height;
	view.width = p_width;
	view.minDepth = p_minD;
	view.maxDepth = p_maxD;
	view.y = (view.height < 0) ? -view.height : 0; // doing this so Vulkan Coordinates reflect OpenGL's
	vkCmdSetViewport(p_cmdbfr, 0, 1, &view);
}

void CVulkanCore::SetScissors(VkCommandBuffer p_cmdBfr, uint32_t p_offX, uint32_t p_offY, uint32_t p_width, uint32_t p_height)
{
	// update dynamic scissor state
	VkRect2D l_vkScissor{};
	l_vkScissor.extent.width = p_width;
	l_vkScissor.extent.height = p_height;
	l_vkScissor.offset.x = p_offX;
	l_vkScissor.offset.y = p_offY;
	vkCmdSetScissor(p_cmdBfr, 0, 1, &l_vkScissor);
}

bool CVulkanCore::CreateGraphicsPipeline(const ShaderPaths& p_shaderPaths, Pipeline& pData, std::string p_debugName)
{
	pData.vertexShader = VK_NULL_HANDLE;
	if (p_shaderPaths.shaderpath_vertex == "" || !LoadShader(p_shaderPaths.shaderpath_vertex.string().c_str(), pData.vertexShader))
		return false;

	pData.fragmentShader = VK_NULL_HANDLE;
	if (p_shaderPaths.shaderpath_fragment != "")
	{
		if (!LoadShader(p_shaderPaths.shaderpath_fragment.string().c_str(), pData.fragmentShader))
			return false;
	}

	VkPipelineVertexInputStateCreateInfo vertInInfo{};
	vertInInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (pData.vertexAttributeDesc.empty())
	{
		vertInInfo.pVertexBindingDescriptions		= VK_NULL_HANDLE;
		vertInInfo.vertexBindingDescriptionCount	= 0;
		vertInInfo.pVertexAttributeDescriptions		= VK_NULL_HANDLE;
		vertInInfo.vertexAttributeDescriptionCount	= 0;
	}
	else
	{
		vertInInfo.pVertexBindingDescriptions		= &pData.vertexInBinding;
		vertInInfo.vertexBindingDescriptionCount	= 1;
		vertInInfo.pVertexAttributeDescriptions		= pData.vertexAttributeDesc.data();
		vertInInfo.vertexAttributeDescriptionCount	= (uint32_t)pData.vertexAttributeDesc.size();
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;
	VkPipelineShaderStageCreateInfo shaderCreateInfo{};
	shaderCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderCreateInfo.stage						= VK_SHADER_STAGE_VERTEX_BIT;
	shaderCreateInfo.module						= pData.vertexShader;
	shaderCreateInfo.pName						= "main";
	shaderStageCreateInfo.push_back(shaderCreateInfo);

	if (pData.fragmentShader != VK_NULL_HANDLE)
	{
		VkPipelineShaderStageCreateInfo shaderCreateInfo{};
		shaderCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.stage					= VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderCreateInfo.module					= pData.fragmentShader;
		shaderCreateInfo.pName					= "main";
		shaderStageCreateInfo.push_back(shaderCreateInfo);
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology			= (pData.isWireframe) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; ;

	VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
	rasterCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterCreateInfo.flags						= 0;
	rasterCreateInfo.depthClampEnable			= VK_FALSE;
	rasterCreateInfo.rasterizerDiscardEnable	= VK_FALSE;
	rasterCreateInfo.polygonMode				= VK_POLYGON_MODE_FILL;//(pData.isWireframe) ? VK_POLYGON_MODE_LINE: VK_POLYGON_MODE_FILL;
	rasterCreateInfo.cullMode					= pData.cullMode;
	rasterCreateInfo.frontFace					= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterCreateInfo.depthBiasEnable			= VK_FALSE;
	rasterCreateInfo.lineWidth					= 1.0f;

	VkViewport viewport{};
	viewport.x									= 0.0f;
	viewport.y									= 0.0f;
	viewport.width								= (float)m_renderWidth;
	viewport.height								= (float)m_renderHeight;
	viewport.minDepth							= 0.0f;
	viewport.maxDepth							= 1.0f;

	VkRect2D scissor{};
	scissor.offset.x							= 0;
	scissor.offset.y							= 0;
	scissor.extent.width						= m_renderWidth;
	scissor.extent.height						= m_renderHeight;

	VkPipelineViewportStateCreateInfo viewportreateInfo{};
	viewportreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportreateInfo.flags						= 0;
	viewportreateInfo.viewportCount				= 1;
	viewportreateInfo.pViewports				= &viewport;
	viewportreateInfo.scissorCount				= 1;
	viewportreateInfo.pScissors					= &scissor;

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
	multisampleCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.flags					= 0;
	multisampleCreateInfo.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
	multisampleCreateInfo.sampleShadingEnable	= VK_FALSE;
	multisampleCreateInfo.minSampleShading		= 0.0f;
	multisampleCreateInfo.pSampleMask			= nullptr;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable		= VK_FALSE;

	Renderpass rpData = pData.renderpassData;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	if (rpData.isDepthAttached() == true)
	{
		depthStencilInfo.flags					= 0;
		depthStencilInfo.depthTestEnable		= pData.enableDepthTest; //VK_TRUE;
		depthStencilInfo.depthWriteEnable		= pData.enableDepthWrite; //VK_TRUE;
		depthStencilInfo.depthCompareOp			= pData.depthCmpOp;
		depthStencilInfo.depthBoundsTestEnable	= VK_FALSE;
		depthStencilInfo.stencilTestEnable		= VK_FALSE;
		depthStencilInfo.back.failOp			= VK_STENCIL_OP_KEEP;
		depthStencilInfo.back.passOp			= VK_STENCIL_OP_KEEP;
		depthStencilInfo.back.compareOp			= VK_COMPARE_OP_ALWAYS;
		depthStencilInfo.front					= depthStencilInfo.back;
	}

	uint32_t colorAttachCount = (uint32_t)rpData.colorAttachmentRefList.size();
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendState(colorAttachCount, VkPipelineColorBlendAttachmentState{});
	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
	if (colorAttachCount > 0) // color attachment are in use
	{
		for (unsigned int i = 0; i < colorAttachCount; i++)
		{
			if (pData.enableBlending == true)
			{
				colorBlendState[i].blendEnable			= VK_TRUE;
				colorBlendState[i].srcColorBlendFactor	= VK_BLEND_FACTOR_SRC_ALPHA;
				colorBlendState[i].dstColorBlendFactor	= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendState[i].colorBlendOp			= VK_BLEND_OP_ADD;
				colorBlendState[i].srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendState[i].dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
				colorBlendState[i].alphaBlendOp			= VK_BLEND_OP_ADD;
				colorBlendState[i].colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			}
			else
			{
				colorBlendState[i].blendEnable			= VK_FALSE;
				colorBlendState[i].colorWriteMask		= 0xf;
			}
		}

		colorBlendCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.pAttachments		= colorBlendState.data();
		colorBlendCreateInfo.attachmentCount	= (uint32_t)colorBlendState.size();
	}	

	VkDynamicState dynamicState[2]				= { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicCreateInfo{};
	dynamicCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicCreateInfo.flags						= 0;
	dynamicCreateInfo.dynamicStateCount			= 2;
	dynamicCreateInfo.pDynamicStates			= dynamicState;

	VkGraphicsPipelineCreateInfo gfxPipelineCreateInfo{};
	gfxPipelineCreateInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	gfxPipelineCreateInfo.stageCount			= (uint32_t)shaderStageCreateInfo.size();
	gfxPipelineCreateInfo.pStages				= shaderStageCreateInfo.data();
	gfxPipelineCreateInfo.pVertexInputState		= &vertInInfo;
	gfxPipelineCreateInfo.pInputAssemblyState	= &inputAssemblyCreateInfo;	
	gfxPipelineCreateInfo.pViewportState		= &viewportreateInfo;				
	gfxPipelineCreateInfo.pRasterizationState	= &rasterCreateInfo;			
	gfxPipelineCreateInfo.pMultisampleState		= &multisampleCreateInfo;		
	gfxPipelineCreateInfo.pDepthStencilState	= &depthStencilInfo;
	gfxPipelineCreateInfo.pColorBlendState		= &colorBlendCreateInfo;			
	gfxPipelineCreateInfo.pDynamicState			= &dynamicCreateInfo;				
	gfxPipelineCreateInfo.layout				= pData.pipeLayout;
	gfxPipelineCreateInfo.renderPass			= pData.renderpassData.renderpass;

	VkResult res = vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &gfxPipelineCreateInfo, nullptr, &pData.pipeline);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateGraphicsPipelines failed: " << res << std::endl;
		return false;
	}

	SetDebugName((uint64_t)pData.pipeline, VK_OBJECT_TYPE_PIPELINE, p_debugName.c_str());

	return true;
}

bool CVulkanCore::CreateComputePipeline(const ShaderPaths& p_shaderPaths, Pipeline& p_pData, std::string p_debugName)
{
	// load shader and get shader module
	p_pData.computeShader = VK_NULL_HANDLE;
	if (!LoadShader(p_shaderPaths.shaderpath_compute.string().c_str(), p_pData.computeShader))
		return false;

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
	pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineShaderStageCreateInfo.flags = 0;
	pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineShaderStageCreateInfo.module = p_pData.computeShader;
	pipelineShaderStageCreateInfo.pName = "main";
	pipelineShaderStageCreateInfo.pSpecializationInfo = nullptr;

	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.layout = p_pData.pipeLayout;
	computePipelineInfo.stage = pipelineShaderStageCreateInfo;
	VkResult res = vkCreateComputePipelines(m_vkDevice, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &p_pData.pipeline);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateComputePipelines failed: " << res << std::endl;
		return false;
	}

	SetDebugName((uint64_t)p_pData.pipeline, VK_OBJECT_TYPE_PIPELINE, p_debugName.c_str());

	return true;
}

void CVulkanCore::DestroyPipeline(Pipeline& p_pipeline)
{
	if (p_pipeline.vertexShader != VK_NULL_HANDLE)
		vkDestroyShaderModule(m_vkDevice, p_pipeline.vertexShader, nullptr);

	if (p_pipeline.fragmentShader != VK_NULL_HANDLE)
		vkDestroyShaderModule(m_vkDevice, p_pipeline.fragmentShader, nullptr);

	if (p_pipeline.computeShader != VK_NULL_HANDLE)
		vkDestroyShaderModule(m_vkDevice, p_pipeline.computeShader, nullptr);
	
	DestroyPipelineLayout(p_pipeline.pipeLayout);

	vkDestroyPipeline(m_vkDevice, p_pipeline.pipeline, nullptr);
}

bool CVulkanCore::CreateSemaphor(VkSemaphore& p_semaphore, std::string p_dbgName)
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkResult res = vkCreateSemaphore(m_vkDevice, &semaphoreCreateInfo, nullptr, &p_semaphore);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateSemaphore failed: " << res << std::endl;
		return false;
	}

	SetDebugName((uint64_t)p_semaphore, VkObjectType::VK_OBJECT_TYPE_SEMAPHORE, p_dbgName.c_str());

	return true;
}

void CVulkanCore::DestroySemaphore(VkSemaphore p_semaphore)
{
	vkDestroySemaphore(m_vkDevice, p_semaphore, nullptr);
}

bool CVulkanCore::CreateFence(VkFenceCreateFlags p_flags, VkFence& p_fence, std::string p_dbgName)
{
	VkFenceCreateInfo fenceinfo{};
	fenceinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceinfo.flags = p_flags;
	VkResult res = vkCreateFence(m_vkDevice, &fenceinfo, nullptr, &p_fence);
	if (res != VK_SUCCESS)
	{
		std::cerr << "CreateFence failed: " << res << std::endl;
		return false;
	}

	SetDebugName((uint64_t)p_fence, VkObjectType::VK_OBJECT_TYPE_FENCE, p_dbgName.c_str());

	return true;
}

bool CVulkanCore::WaitFence(VkFence& p_fence)
{
	VkResult res = vkWaitForFences(m_vkDevice, 1, &p_fence, VK_TRUE, UINT64_MAX);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkWaitForFences failed: " << res << std::endl;
		return false;
	}
	return true;
}

bool CVulkanCore::ResetFence(VkFence& p_fence)
{
	VkResult res = vkResetFences(m_vkDevice, 1, &p_fence);
	if (res != VK_SUCCESS)
	{
		std::cerr << "ResetFence failed: " << res << std::endl;
		return false;
	}
	return true;
}

void CVulkanCore::DestroyFence(VkFence p_fence)
{
	vkDestroyFence(m_vkDevice, p_fence, nullptr);
}

bool CVulkanCore::BeginCommandBuffer(VkCommandBuffer& p_cmdBfr, const char* p_debugMarker)
{
	VkCommandBufferBeginInfo l_cmdBufferBeginInfo{};
	l_cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	l_cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult res = vkBeginCommandBuffer(p_cmdBfr, &l_cmdBufferBeginInfo);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkBeginCommandBuffer failed: " << res << std::endl;
		return false;
	}
	BeginDebugMarker(p_cmdBfr, p_debugMarker);
	return true;
}

bool CVulkanCore::EndCommandBuffer(VkCommandBuffer& p_cmdBfr)
{
	EndDebugMarker(p_cmdBfr);

	VkResult res = vkEndCommandBuffer(p_cmdBfr);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkEndCommandBuffer failed: " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::SubmitCommandbuffer(VkQueue p_queue, VkSubmitInfo* p_subInfoList, uint32_t p_subInfoCount, VkFence p_fence)
{
	VkResult res = vkQueueSubmit(p_queue, p_subInfoCount, p_subInfoList, p_fence);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkQueueSubmit failed " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::ResetCommandBuffer(VkCommandBuffer p_cmdBfr)
{
	VkResult res = vkResetCommandBuffer(p_cmdBfr, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	if(res != VK_SUCCESS)
	{
		std::cerr << "vkResetCommandBuffer failed " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::WaitToFinish(VkQueue p_queue)
{
	VkResult res = vkQueueWaitIdle(p_queue);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkQueueWaitIdle failed " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::IssueLayoutBarrier(VkImageLayout p_new, Image& p_image, VkCommandBuffer p_cmdBfr)
{
	p_image.descInfo.imageLayout = p_new;
	if (p_new == p_image.curLayout)
		return;

	IssueImageLayoutBarrier(p_image.curLayout, p_new, p_image.layerCount, p_image.levelCount, p_image.image, p_image.usage, p_cmdBfr);
	p_image.curLayout = p_new;
}

void CVulkanCore::IssueImageLayoutBarrier(	VkImageLayout p_old, VkImageLayout p_new, 
											uint32_t p_layerCount, uint32_t p_levelCount, 
											VkImage& p_image, VkImageUsageFlags p_usage,
											VkCommandBuffer p_cmdBfr, uint32_t p_baseMipLevel)
{
	VkImageMemoryBarrier imgMemBarrier{};
	imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	// following flags used to transfer one image type to other
	imgMemBarrier.oldLayout = p_old;
	imgMemBarrier.newLayout = p_new;
	// if you wish to transfer the queue family ownership, use the following flags
	imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;					// for now we dont care
	imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;					// for now we dont care
	// setting affected image and specified part
	imgMemBarrier.image = p_image;
	imgMemBarrier.subresourceRange.baseArrayLayer = 0;
	imgMemBarrier.subresourceRange.levelCount = p_levelCount;
	imgMemBarrier.subresourceRange.layerCount = p_layerCount;
	imgMemBarrier.subresourceRange.baseMipLevel = p_baseMipLevel;
	
	if (p_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		if (p_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
			p_usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		{
			imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	VkPipelineStageFlags src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dst = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	
	// setting up operations that need to happen before the barrier
	switch (p_old)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		imgMemBarrier.srcAccessMask = 0;
		src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		src = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	default:
		imgMemBarrier.srcAccessMask = 0;
		src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	}

	// setting up operations that need to happen after the barrier
	switch (p_new)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		dst = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_GENERAL:
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		imgMemBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		dst = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dst = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		dst = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	default:
		dst = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		imgMemBarrier.dstAccessMask = 0;
		break;
	}

	vkCmdPipelineBarrier(p_cmdBfr,
		src, dst,
		0,
		0, nullptr,
		0, nullptr,
		1, &imgMemBarrier);
}

void CVulkanCore::IssueBufferBarrier(
	VkAccessFlags p_srcAcc, VkAccessFlags p_dstAcc, 
	VkPipelineStageFlags p_srcStg, VkPipelineStageFlags p_dstStg, 
	VkBuffer& p_buffer, VkCommandBuffer p_cmdBfr)
{
	VkBufferMemoryBarrier bufMemBarrier{};
	bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufMemBarrier.size = VK_WHOLE_SIZE;
	bufMemBarrier.buffer = p_buffer;
	bufMemBarrier.srcAccessMask = p_srcAcc;
	bufMemBarrier.dstAccessMask = p_dstAcc;

	vkCmdPipelineBarrier(
		p_cmdBfr,
		p_srcStg,p_dstStg,
		0, 0, nullptr,
		1, &bufMemBarrier,
		0, nullptr);
}

bool CVulkanCore::IsFormatSupported(VkFormat p_format, VkFormatFeatureFlags p_featureflag)
{
	VkFormatProperties l_vkFormatProperties{};
	vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, p_format, &l_vkFormatProperties);
	if (!(l_vkFormatProperties.optimalTilingFeatures & p_featureflag))
	{
		std::cerr << "Format not supported: " << p_format << std::endl;
		return false;
	}

	return true;
}

uint32_t CVulkanCore::FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& p_physicalDeviceMemProp,
	const VkMemoryRequirements* p_memoryReq, const VkMemoryPropertyFlags p_requiredMemPropFlags)
{
	uint32_t l_uiMemoryIndex = UINT32_MAX;

	// looping through all offered memory types
	for (uint32_t i = 0; i < p_physicalDeviceMemProp.memoryTypeCount; ++i)
	{
		// tells us which type of memory location (GPU, RAM etc) can the memory be put into
		if (p_memoryReq->memoryTypeBits & (1 << i))
		{
			// if so, check this memory type of gpu offered types is if exact format as we need need ie: required properties
			if ((p_physicalDeviceMemProp.memoryTypes[i].propertyFlags & p_requiredMemPropFlags) == p_requiredMemPropFlags)
			{
				return i;
			}
		}
	}

	assert(l_uiMemoryIndex != UINT32_MAX);

	return UINT32_MAX;
}

bool CVulkanCore::CreateImage(VkImageCreateInfo p_imageCreateInfo, VkImage& p_image)
{
	VkResult res = vkCreateImage(m_vkDevice, &p_imageCreateInfo, nullptr, &p_image);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateImage failed " << res << std::endl;
		return false;
	}
		
	return true;
}

bool CVulkanCore::AllocateImageMemory(VkImage p_image, VkMemoryPropertyFlags p_memFlags, VkDeviceMemory& p_devMem)
{
	// Memory requirements for images
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(m_vkDevice, p_image, &memReq);

	// memory type index
	uint32_t memIndex = FindMemoryTypeIndex(m_vkPhysicalDeviceMemProp, &memReq, p_memFlags);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memReq.size;
	memAllocInfo.memoryTypeIndex = memIndex;

	// allocate memory
	VkResult res = vkAllocateMemory(m_vkDevice, &memAllocInfo, nullptr, &p_devMem);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkAllocateMemory for image failed " << res << std::endl;
		return false;
	}
	return true;

}

bool CVulkanCore::BindImageMemory(VkImage& p_image, VkDeviceMemory& p_devMem)
{
	VkResult res = vkBindImageMemory(m_vkDevice, p_image, p_devMem, 0);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkBindImageMemory failed " << res << std::endl;
		return false;
	}
	return true;

}

bool CVulkanCore::CreateImagView(VkImageUsageFlags p_usage, VkImage p_image, VkFormat p_format, 
	VkImageViewType p_viewType, uint32_t p_levelCount, VkImageView& p_imgView)
{
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (p_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	else if (p_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	
	VkImageViewCreateInfo imgviewCreateInfo{};
	imgviewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgviewCreateInfo.image = p_image;
	imgviewCreateInfo.viewType = p_viewType;
	imgviewCreateInfo.format = p_format;
	imgviewCreateInfo.subresourceRange.aspectMask = aspectFlag;
	imgviewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgviewCreateInfo.subresourceRange.layerCount = (p_viewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;
	imgviewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgviewCreateInfo.subresourceRange.levelCount = p_levelCount;
	VkResult res = vkCreateImageView(m_vkDevice, &imgviewCreateInfo, nullptr, &p_imgView);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateImageView failed " << res << std::endl;
		return false;
	}
	return true;
}

void CVulkanCore::DestroyImageView(VkImageView p_imageView)
{
	vkDestroyImageView(m_vkDevice, p_imageView, nullptr);
}

void CVulkanCore::DestroyImage(VkImage p_image)
{
	vkDestroyImage(m_vkDevice, p_image, nullptr);
}

void CVulkanCore::ClearImage(VkCommandBuffer p_cmdBfr, VkImage p_src, VkImageLayout p_srclayout, VkClearValue p_clearValue)
{
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.baseArrayLayer = 0;
	range.layerCount = 1;
	range.levelCount = 1;

	vkCmdClearColorImage(p_cmdBfr, p_src, p_srclayout, &p_clearValue.color, 1, &range);
}

void CVulkanCore::CopyImage(VkCommandBuffer p_cmdBfr, VkImage p_src, VkImageLayout p_srclayout, 
	VkImage p_dest, VkImageLayout p_destLayout, uint32_t p_width, uint32_t p_height)
{
	uint32_t regionCount = 1;
	// Assuming few things here. Will keep improving this based on need
	VkImageCopy imageCopyRegion{};
	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.layerCount = 1;
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.layerCount = 1;
	imageCopyRegion.extent.width = p_width;
	imageCopyRegion.extent.height = p_height;
	imageCopyRegion.extent.depth = 1;
	vkCmdCopyImage(p_cmdBfr, p_src, p_srclayout, p_dest, p_destLayout, 1, &imageCopyRegion);
}

void CVulkanCore::BlitImage(VkCommandBuffer p_cmdBfr, VkImageBlit p_imgBlit, VkImage p_srcImage, 
	VkImageLayout p_srcImageLayout, VkImage p_dstImage, VkImageLayout p_dstImageLayout, VkFormat p_imgForamt)
{
	// Check for device support for Linear Blitting
	VkFormatProperties props{};
	vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, p_imgForamt,&props);
	if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		std::cerr << "CVulkanCore::BlitImage Filed since physical device does not support Linear Filtering on Blit operation" << std::endl; 
		return;
	}

	vkCmdBlitImage(p_cmdBfr, p_srcImage, p_srcImageLayout, p_dstImage, p_dstImageLayout, 1, &p_imgBlit, VK_FILTER_LINEAR);
}

bool CVulkanCore::CreateSampler(Sampler& p_sampler)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = p_sampler.filter;
	samplerInfo.minFilter = p_sampler.filter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	// anistropy enabled
	if (p_sampler.maxAnisotropy > 1.0f && p_sampler.maxAnisotropy < 16 && (int)(p_sampler.maxAnisotropy) % 2 == 0)
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = p_sampler.maxAnisotropy;
	}

	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	// mip mapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 100.0f;

	VkResult res = vkCreateSampler(m_vkDevice, &samplerInfo, nullptr, &p_sampler.descInfo.sampler);
	if (res != VK_SUCCESS)
	{
		std::cerr << "CVulkanCore::CreateSampler: vkCreateSampler failed: " << res << std::endl;
		return false;
	}

	return true;
}

void CVulkanCore::DestroySampler(VkSampler p_sampler)
{
	vkDestroySampler(m_vkDevice, p_sampler, nullptr);
}

bool CVulkanCore::CreateBuffer(VkBufferCreateInfo p_bufferCreateInfo, VkBuffer& p_buffer)
{
	VkResult res = vkCreateBuffer(m_vkDevice, &p_bufferCreateInfo, nullptr, &p_buffer);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkCreateBuffer failed " << res << std::endl;
		return false;
	}

	return true;
}

bool CVulkanCore::AllocateBufferMemory(VkBuffer p_buffer, VkMemoryPropertyFlags p_memFlags, VkDeviceMemory& p_devMem, size_t& p_reqSize)
{
	VkMemoryRequirements bufferMemReq;
	vkGetBufferMemoryRequirements(m_vkDevice, p_buffer, &bufferMemReq);
	p_reqSize = bufferMemReq.size;

	uint32_t memIndex = FindMemoryTypeIndex(m_vkPhysicalDeviceMemProp, &bufferMemReq, p_memFlags);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = bufferMemReq.size;
	memoryAllocateInfo.memoryTypeIndex = memIndex;

	VkResult res = vkAllocateMemory(m_vkDevice, &memoryAllocateInfo, nullptr, &p_devMem);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkAllocateMemory failed " << res << std::endl;
		return false;
	}
	return true;
}

bool CVulkanCore::BindBufferMemory(VkBuffer& p_buffer, VkDeviceMemory& p_devMem)
{
	VkResult res = vkBindBufferMemory(m_vkDevice, p_buffer, p_devMem, 0);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkBindBufferMemory for buffer failed " << res << std::endl;
		return false;
	}
	return true;
}

void CVulkanCore::FreeDeviceMemory(VkDeviceMemory& p_devMem)
{
	vkFreeMemory(m_vkDevice, p_devMem, nullptr);
	p_devMem = VK_NULL_HANDLE;
}

void CVulkanCore::DestroyBuffer(VkBuffer &p_buffer)
{
	vkDestroyBuffer(m_vkDevice, p_buffer, nullptr);
	p_buffer = VK_NULL_HANDLE;

}

bool CVulkanCore::MapMemory(Buffer p_buffer, bool p_flushMemRanges, void** p_data, std::vector<VkMappedMemoryRange>* p_memRanges)
{
	VkResult res = vkMapMemory(m_vkDevice, p_buffer.devMem, p_buffer.descInfo.offset, p_buffer.reqMemSize, 0, p_data);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkMapMemory failed " << res << std::endl;
		return false;
	}

	if (p_flushMemRanges == true && p_memRanges!= nullptr)
	{
		VkMappedMemoryRange range{};
		range.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = p_buffer.devMem;
		range.size = p_buffer.reqMemSize; // we do this because VK_WHOLE_SIZE is raising a validation error because vulkan driver (or loader ?) is picking up VK_WHOLE_SIZE as device range and not required memory size discovered during device buffer memory creation
		p_memRanges->push_back(range);
	}

	return true;
}

bool CVulkanCore::FlushMemoryRanges(std::vector<VkMappedMemoryRange>* p_memRanges)
{
	if (p_memRanges == nullptr)
	{
		std::cerr << "CVulkanCore::FlushMemoryRanges Error: FlushMemoryRanges failed becasue p_memRanges is nullptr " << std::endl;
		return false;
	}

	//we are 
	for (auto& mapped : *p_memRanges)
	{
		VkResult res = vkFlushMappedMemoryRanges(m_vkDevice, 1, &mapped);
		if (res != VK_SUCCESS)
		{
			std::cerr << "CVulkanCore::FlushMemoryRanges Error: CVulkanCore::FlushMemoryRanges " << mapped.size << std::endl;
			return false;
		}
	}

	return true;
}

void CVulkanCore::UnMapMemory(Buffer p_buffer)
{
	vkUnmapMemory(m_vkDevice, p_buffer.devMem);
}

bool CVulkanCore::WriteToBuffer(void* p_data, Buffer p_buffer, bool p_bFlush)
{
	uint8_t* data = nullptr;
	std::vector<VkMappedMemoryRange> ranges;
	if (!MapMemory(p_buffer, p_bFlush, (void**)&data, &ranges))
		return false;

	memcpy(data, p_data, (size_t)p_buffer.descInfo.range);

	if (p_bFlush == true)
	{
		if (!FlushMemoryRanges(&ranges))
			return false;
	}

	UnMapMemory(p_buffer);

	return true;
}

bool CVulkanCore::ReadFromBuffer(void* p_data, Buffer p_buffer)
{
	// Make device writes visible to the host
	void* mapped;

	VkResult res = vkMapMemory(m_vkDevice, p_buffer.devMem, 0, VK_WHOLE_SIZE, 0, &mapped);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkMapMemory failed " << res << std::endl;
		return false;
	}

	VkMappedMemoryRange mappedRange{};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = p_buffer.devMem;
	mappedRange.offset = 0;
	mappedRange.size = VK_WHOLE_SIZE;
	res = vkInvalidateMappedMemoryRanges(m_vkDevice, 1, &mappedRange);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkInvalidateMappedMemoryRanges failed " << res << std::endl;
		return false;
	}

	// Copy to output
	memcpy(p_data, mapped, (size_t)p_buffer.descInfo.range);

	vkUnmapMemory(m_vkDevice, p_buffer.devMem);

	return true;
}

bool CVulkanCore::UploadFromHostToDevice(Buffer& p_staging, Buffer& p_dest, VkCommandBuffer& p_cmdBfr)
{
	// check flag; if flag set to allocated data on GPU memory
	if (!p_dest.memPropFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		std::cerr << "UploadStagingToBuffer failed: bad memory property flag" << std::endl;
		return false;
	}
	VkBufferCopy cpyRgn = {};
	cpyRgn.size = p_staging.descInfo.range;

	InsertMarker(p_cmdBfr, "Upload From Host To Device");
	vkCmdCopyBuffer(p_cmdBfr, p_staging.descInfo.buffer, p_dest.descInfo.buffer, 1, &cpyRgn);
	
	return true;
}

void CVulkanCore::UploadFromHostToDevice(Buffer& p_staging, VkImageLayout p_finLayout, Image& p_dest, VkCommandBuffer& p_cmdBfr)
{
	std::vector<VkBufferImageCopy> bcInfoList;

	for (unsigned int i = 0; i < p_dest.layerCount; i++)
	{
		VkBufferImageCopy bcInfo{};
		bcInfo.bufferOffset = i* p_dest.bufOffset;				// point on the buffer at which the pixels start
		bcInfo.bufferRowLength = 0;											// no padding used between rows
		bcInfo.bufferImageHeight = 0;										// no padding used between columns
		bcInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bcInfo.imageSubresource.mipLevel = 0;
		bcInfo.imageSubresource.baseArrayLayer = i;
		bcInfo.imageSubresource.layerCount = 1;
		bcInfo.imageExtent = { p_dest.width, p_dest.height, 1 };
		bcInfoList.push_back(bcInfo);
	}
	// copy bufffer to image
	vkCmdCopyBufferToImage(p_cmdBfr, p_staging.descInfo.buffer, p_dest.image, p_finLayout, (uint32_t)bcInfoList.size(), bcInfoList.data());
}

bool CVulkanCore::ListAvailableInstanceLayers(std::vector<const char*> reqList)
{
	// Enabeling harware supported instance layers
	uint32_t layerCount = 0;
	VkResult res = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkEnumerateInstanceLayerProperties failed: " << res << std::endl;
		return false;
	}

	std::vector<VkLayerProperties> supportedLayerPropList(layerCount);
	res = vkEnumerateInstanceLayerProperties(&layerCount, supportedLayerPropList.data());
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkEnumerateInstanceLayerProperties failed: " << res << std::endl;
		return false;
	}

	std::cout << "listing supported instance layers" << std::endl;
	for (auto& layer : supportedLayerPropList)
	{
		std::cout << "	" << layer.layerName;

		for (auto& reqExtn : reqList)
		{
			if (std::strcmp(reqExtn, layer.layerName) == 0)
			{
				std::cout << "----------------------------LOADED";
			}
		}

		std::cout << std::endl;
	}

	return true;
}

bool CVulkanCore::ListAvailableInstanceExtensions(std::vector<const char*> reqList)
{
	uint32_t extensiionCount = 0;
	VkResult res = vkEnumerateInstanceExtensionProperties(nullptr, &extensiionCount, nullptr);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkEnumerateInstanceExtensionProperties failed: " << res << std::endl;
		return false;
	}

	char* layerNmae = nullptr;
	std::vector<VkExtensionProperties> supportedInstanceExtensionList(extensiionCount);
	res = vkEnumerateInstanceExtensionProperties(layerNmae, &extensiionCount, supportedInstanceExtensionList.data());
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkEnumerateInstanceExtensionProperties failed: " << res << std::endl;
		return false;
	}

	std::cout << "listing supported instance extensions" << std::endl;
	for (auto& exten : supportedInstanceExtensionList)
	{
		std::cout << "	" << exten.extensionName;

		for (auto& reqExtn : reqList)
		{
			if (std::strcmp(reqExtn, exten.extensionName) == 0)
			{
				std::cout << "----------------------------LOADED";
			}
		}

		std::cout << std::endl;
	}

	return true;
}

char* BinaryLoader(const std::string pPath, size_t& pDataSize)
{
	std::ifstream is(pPath, std::ios::binary | std::ios::in | std::ios::ate);
	if (is.is_open())
	{
		pDataSize = (size_t)is.tellg();
		if (pDataSize < 1)
		{
			assert(pDataSize > 0);
			return nullptr;
		}

		is.seekg(0, std::ios::beg);

		// Copy file contents into a buffer
		char* Data = new char[pDataSize]();
		is.read(Data, pDataSize);
		is.close();

		return Data;
	}

	return nullptr;
}